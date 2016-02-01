#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#include <linux/kconfig.h>
#else
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/screen_info.h>
#include <linux/fs.h>
#include <linux/mount.h>


#include "nexus_types.h"
#include "nexus_platform.h"
#include "nexus_display.h"
#include "nexus_composite_output.h"
#include "nexus_component_output.h"
#include "nexus_surface.h"
#include "nexus_graphics2d.h"
#include "nexus_core_utils.h"
#include "nexus_base_mmap.h"
#if NEXUS_HAS_HDMI_OUTPUT
#include "nexus_hdmi_output.h"
#endif
#include <linux/brcmstb/brcmstb.h>
#include "bchp_memc_clients.h"
#include "bchp_memc_arb_0.h"
#if BCHP_P_MEMC_COUNT == 2
#include "bchp_memc_arb_1.h"
#endif
#if BCHP_P_MEMC_COUNT == 3
#include "bchp_memc_arb_1.h"
#include "bchp_memc_arb_2.h"
#endif

struct bcmnexusfb_par {
	  NEXUS_DisplayHandle display;
	  NEXUS_Graphics2DHandle gfx;
	  NEXUS_SurfaceHandle framebuffer;
	  NEXUS_SurfaceMemory mem;
	  u32 pseudo_palette[16];
	  unsigned ref_cnt;
};

static struct fb_fix_screeninfo bcmnexusfb_fix __initdata = {
	.id =		"BCMNEXUS",
	.type =		FB_TYPE_PACKED_PIXELS,
	.accel =	FB_ACCEL_NONE,
};

static struct fb_var_screeninfo bcmnexusfb_var __initdata = {
	.xres = 1920,
	.yres = 1080,
	.xres_virtual = 1920,
	.yres_virtual = 1080,
	.bits_per_pixel = 32,
	.red = {0, 8, 0}, /* beginning of bitfield, length of bitfield, != 0 : Most significant bit is right */
	.green = {8, 8, 0},
	.blue = {16, 8, 0},
	.transp = {24, 0, 0},
	.activate	= FB_ACTIVATE_NOW,
	.height		= 1920,
	.width		= 1080,
};

static int bcmnexusfb_open(struct fb_info *info, int user)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;
	NEXUS_PlatformConfiguration platformConfig;
	/* NEXUS_PlatformSettings platformSettings; */
	NEXUS_DisplaySettings displaySettings;
	NEXUS_SurfaceCreateSettings createSettings;
	NEXUS_GraphicsSettings graphicsSettings;
	NEXUS_VideoFormatInfo videoFormatInfo;
	NEXUS_HdmiOutputHandle hdmiOutput;
	NEXUS_HdmiOutputStatus hdmiStatus;
	NEXUS_Error rc;
	int timeout = 10;

	if(par->ref_cnt){
		goto done;
	}

	NEXUS_Platform_GetConfiguration(&platformConfig);

	NEXUS_Display_GetDefaultSettings(&displaySettings);
	displaySettings.displayType = NEXUS_DisplayType_eAuto;
	displaySettings.format = NEXUS_VideoFormat_e1080p;
	hdmiOutput = platformConfig.outputs.hdmi[0];
	do {
		rc = NEXUS_HdmiOutput_GetStatus(hdmiOutput, &hdmiStatus);
		if (rc == NEXUS_SUCCESS && hdmiStatus.connected) {
			/* Use preferred video format if info is available */
			displaySettings.format = hdmiStatus.preferredVideoFormat;
			NEXUS_VideoFormat_GetInfo(displaySettings.format, &videoFormatInfo);
			printk("Using preferred video format: %d\n", hdmiStatus.preferredVideoFormat);

			/* Limit ourselves to 1080p if the resolution is higher */
			if (videoFormatInfo.width > 1920 || videoFormatInfo.height > 1080) {
				displaySettings.format = NEXUS_VideoFormat_e1080p;
				printk("Overriding preferred video format: %d -> %d\n", hdmiStatus.preferredVideoFormat, displaySettings.format);
			}
			break;
		}
		printk(".");
		msleep(250);
	} while (timeout--);
	par->display = NEXUS_Display_Open(0, &displaySettings);
	if(!par->display) {goto err_display;}
	rc = NEXUS_Display_AddOutput(par->display, NEXUS_HdmiOutput_GetVideoConnector(hdmiOutput));
	if(rc!=NEXUS_SUCCESS) {goto err_output;}
	par->gfx = NEXUS_Graphics2D_Open(0, NULL);
	if(!par->gfx) {goto err_graphics;}

	NEXUS_VideoFormat_GetInfo(displaySettings.format, &videoFormatInfo);
	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width = videoFormatInfo.width;
	createSettings.height = videoFormatInfo.height;
        createSettings.heap = NEXUS_Platform_GetFramebufferHeap(0);
	par->framebuffer = NEXUS_Surface_Create(&createSettings);
	if(par->framebuffer == NULL) {goto err_graphics;}
	rc = NEXUS_Surface_GetMemory(par->framebuffer, &par->mem);
	if(rc!=NEXUS_SUCCESS) {goto err_surface;}

	NEXUS_Display_GetGraphicsSettings(par->display, &graphicsSettings);
	graphicsSettings.enabled = true;
	graphicsSettings.clip.width = createSettings.width;
	graphicsSettings.clip.height = createSettings.height;
	graphicsSettings.sourceBlendFactor = NEXUS_CompositorBlendFactor_eOne;
	graphicsSettings.destBlendFactor = NEXUS_CompositorBlendFactor_eZero;
	/* Fix alpha at 0xFF as we do RGBX here */
	graphicsSettings.constantAlpha = 0xFF;
	NEXUS_Display_SetGraphicsSettings(par->display, &graphicsSettings);
	NEXUS_Display_SetGraphicsFramebuffer(par->display, par->framebuffer);

	info->fix.smem_start = (unsigned long)NEXUS_AddrToOffset(par->mem.buffer);
	info->fix.smem_len = createSettings.height * par->mem.pitch;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = par->mem.pitch;

	info->var.xres = createSettings.width;
	info->var.yres = createSettings.height;
	info->var.xres_virtual = createSettings.width;
	info->var.yres_virtual = createSettings.height;

	info->screen_base = (char __force __iomem *)par->mem.buffer;
	info->screen_size = info->fix.smem_len;

done:
	printk("fb%d: %dx%d - %dx%d - %x, %u\n", info->node, info->var.xres, info->var.yres,
               info->var.xres_virtual, info->var.yres_virtual, info->screen_base, info->screen_size);
	par->ref_cnt++;
	return 0;

err_surface:
	printk("fb%d: err_surface - FAILED\n", info->node);
	NEXUS_Surface_Destroy(par->framebuffer);
err_graphics:
	printk("fb%d: err_graphics - FAILED\n", info->node);
	NEXUS_Display_RemoveAllOutputs(par->display);
err_output:
	printk("fb%d: err_output - FAILED\n", info->node);
	NEXUS_Display_Close(par->display);
err_display:
	printk("fb%d: err_display - FAILED\n", info->node);
	return -ENODEV;
}

static int bcmnexusfb_release(struct fb_info *info, int user)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;

	if (par->ref_cnt == 0) {
		/* Do nothing if we were already done */
		return 0;
	}

	par->ref_cnt--;

	if (par->ref_cnt) {
		return 0;
	}

	/* NEXUS_Display_RemoveAllOutputs(par->display); - not needed. */
	NEXUS_Display_Close(par->display);
	NEXUS_Graphics2D_Close(par->gfx);
	NEXUS_Surface_Destroy(par->framebuffer);
	return 0;
}

static int bcmnexusfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp, struct fb_info *info)
{
	if (regno >= 256)  /* no. of hw registers */
	  return -EINVAL;

	/* grayscale works only partially under directcolor */
	if (info->var.grayscale) {
		/* grayscale = 0.30*R + 0.59*G + 0.11*B */
		red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
	}

	if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
	    info->fix.visual == FB_VISUAL_DIRECTCOLOR) {
		u32 v;
		if (regno >= 16)
			return -EINVAL;

		v = (red << info->var.red.offset) |
			(green << info->var.green.offset) |
			(blue << info->var.blue.offset) |
			(transp << info->var.transp.offset);

		((u32*)(info->pseudo_palette))[regno] = v;
	}

	return 0;
}

static int bcmnexusfb_blank(int blank_mode, struct fb_info *info)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;
	NEXUS_GraphicsSettings graphicsSettings;

	printk("fb%d: blank %d\n", info->node, blank_mode);

	NEXUS_Display_GetGraphicsSettings(par->display, &graphicsSettings);
	graphicsSettings.visible = blank_mode==FB_BLANK_UNBLANK?true:false;
	NEXUS_Display_SetGraphicsSettings(par->display, &graphicsSettings);

	return 0;
}

void bcmnexusfb_fillrect(struct fb_info *p, const struct fb_fillrect *region)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)p->par;
	NEXUS_Graphics2DFillSettings fillSettings;

	NEXUS_Graphics2D_GetDefaultFillSettings(&fillSettings);
	fillSettings.surface = par->framebuffer;
	fillSettings.rect.x = region->dx;
	fillSettings.rect.y = region->dy;
	fillSettings.rect.width = region->width;
	fillSettings.rect.height = region->height;
	fillSettings.color = region->color;

	printk("fb%d: fillrect %d,%d %d,%d\n", p->node, region->dx, region->dy, region->width, region->height);

	NEXUS_Graphics2D_Fill(par->gfx, &fillSettings);
}

void bcmnexusfb_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)p->par;
	NEXUS_Graphics2DBlitSettings blitSettings;

	NEXUS_Graphics2D_GetDefaultBlitSettings(&blitSettings);

	blitSettings.source.surface = par->framebuffer;
	blitSettings.source.rect.x = area->sx;
	blitSettings.source.rect.y = area->sy;
	blitSettings.source.rect.width = area->width;
	blitSettings.source.rect.height = area->height;

	blitSettings.output.surface = par->framebuffer;
	blitSettings.output.rect.x = area->dx;
	blitSettings.output.rect.y = area->dy;
	blitSettings.output.rect.width = area->width;
	blitSettings.output.rect.height = area->height;

	blitSettings.colorOp = NEXUS_BlitColorOp_eCopySource;
	blitSettings.alphaOp = NEXUS_BlitAlphaOp_eCopySource;

	printk("fb%d: copyarea %d,%d %d,%d\n", p->node, area->dx, area->dy, area->width, area->height);

	NEXUS_Graphics2D_Blit(par->gfx, &blitSettings);
}

void bcmnexusfb_imageblit(struct fb_info *p, const struct fb_image *image)
{
	/*TODO : This is required only for frambuffer console.
	  We do not currently support it. Add this when required*/
}

int bcmnexusfb_sync(struct fb_info *info)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;

	printk("fb%d: sync\n", info->node);

	NEXUS_Surface_Flush(par->framebuffer);
	return 0;
}

static struct fb_ops bcmnexusfb_ops = {
	.owner			= THIS_MODULE,
	.fb_open		= bcmnexusfb_open,
	.fb_release		= bcmnexusfb_release,
	.fb_setcolreg	= bcmnexusfb_setcolreg,
	.fb_blank		= bcmnexusfb_blank,
	.fb_fillrect	= bcmnexusfb_fillrect,
	.fb_copyarea	= bcmnexusfb_copyarea,
	.fb_imageblit	= bcmnexusfb_imageblit,
	.fb_sync		= bcmnexusfb_sync,
};

static int __init bcmnexusfb_probe (struct platform_device *pdev)
{
	struct fb_info *info;
	struct bcmnexusfb_par *par;

	/*
	 * Dynamically allocate info and par
	*/
	info = framebuffer_alloc(sizeof(struct bcmnexusfb_par), &pdev->dev);

	if (!info) {
	    /* goto error path */
	}

	par = info->par;
	par->ref_cnt = 0;

	info->screen_base = NULL;

	info->fbops = &bcmnexusfb_ops;
	info->var = bcmnexusfb_var;
	info->fix = bcmnexusfb_fix;

	info->pseudo_palette = par->pseudo_palette; /* The pseudopalette is an
						 * 16-member array
						 */

	info->flags = FBINFO_DEFAULT |
	    FBINFO_HWACCEL_COPYAREA |
	    FBINFO_HWACCEL_FILLRECT |
	    FBINFO_HWACCEL_IMAGEBLIT;

	fb_alloc_cmap(&info->cmap, 256, 0);

	if (register_framebuffer(info) < 0)
	return -EINVAL;

	printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node,
	   info->fix.id);

	platform_set_drvdata(pdev, info);

	return 0;
}

static int __exit bcmnexusfb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		/* ... */
		framebuffer_release(info);
	}

	return 0;
}

static struct platform_driver bcmnexusfb_driver = {
	.probe = bcmnexusfb_probe,
	.remove = bcmnexusfb_remove,
	.driver = {
	.name = "bcmnexusfb",
	},
};

static struct platform_device *bcmnexusfb_device;

static int __init bcmnexusfb_init(void)
{
	int ret;

	ret = platform_driver_register(&bcmnexusfb_driver);

	if (!ret) {
		bcmnexusfb_device = platform_device_alloc("bcmnexusfb", 0);

		if(bcmnexusfb_device) {
			ret = platform_device_add(bcmnexusfb_device);
		} else {
			ret = -ENOMEM;
		}

		if (ret) {
			platform_device_put(bcmnexusfb_device);
			platform_driver_unregister(&bcmnexusfb_driver);
		}
	}

	return ret;
}

static void __exit bcmnexusfb_exit(void)
{
	platform_device_unregister(bcmnexusfb_device);
	platform_driver_unregister(&bcmnexusfb_driver);
}


module_init(bcmnexusfb_init);
module_exit(bcmnexusfb_exit);

MODULE_LICENSE("GPL");
