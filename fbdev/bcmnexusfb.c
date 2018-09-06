#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
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
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
#include "nexus_composite_output.h"
#endif
#include <linux/brcmstb/brcmstb.h>

static int debug_verbose = 0;

struct bcmnexusfb_token {
	NEXUS_DisplayHandle display;
	NEXUS_SurfaceHandle framebuffer;
	NEXUS_SurfaceHandle fb0;
	NEXUS_SurfaceHandle fb1;
	NEXUS_SurfaceMemory mem;
};

struct bcmnexusfb_par {
	NEXUS_Graphics2DHandle gfx;
	struct bcmnexusfb_token hd;
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	struct bcmnexusfb_token sd;
#endif
	u32 pseudo_palette[16];
	unsigned ref_cnt;
};

static struct fb_fix_screeninfo bcmnexusfb_fix __initdata = {
	.id = "BCMNEXUS",
	.type = FB_TYPE_PACKED_PIXELS,
	.accel = FB_ACCEL_NONE,
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
	.activate = FB_ACTIVATE_NOW|FB_ACTIVATE_FORCE,
	.height = 1920,
	.width = 1080,
};

static struct fb_info *bcmfb;

static int bcmnexusfb_open(struct fb_info *info, int user)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;
	NEXUS_PlatformConfiguration platformConfig;
	NEXUS_DisplaySettings displaySettings;
	NEXUS_SurfaceCreateSettings createSettings;
	NEXUS_GraphicsSettings graphicsSettings;
	NEXUS_Graphics2DSettings gfxSettings;
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

	if (timeout > 0) {
		par->hd.display = NEXUS_Display_Open(0, &displaySettings);
		if(!par->hd.display) goto err_display;
		rc = NEXUS_Display_AddOutput(par->hd.display, NEXUS_HdmiOutput_GetVideoConnector(hdmiOutput));
		if(rc!=NEXUS_SUCCESS) goto err_output;
	} else
		par->hd.display = NULL;

	par->gfx = NEXUS_Graphics2D_Open(0, NULL);
	if(!par->gfx) goto err_graphics;
	NEXUS_Graphics2D_GetSettings(par->gfx, &gfxSettings);
	gfxSettings.pollingCheckpoint = true;
	NEXUS_Graphics2D_SetSettings(par->gfx, &gfxSettings);

	NEXUS_VideoFormat_GetInfo(displaySettings.format, &videoFormatInfo);
	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width * 2;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 2 * 4;
	createSettings.heap        = NEXUS_Platform_GetFramebufferHeap(0);
	par->hd.framebuffer        = NEXUS_Surface_Create(&createSettings);
	if(par->hd.framebuffer == NULL) goto err_graphics;
	rc = NEXUS_Surface_GetMemory(par->hd.framebuffer, &par->hd.mem);
	if(rc!=NEXUS_SUCCESS) goto err_surface;

	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 4;
	createSettings.pMemory     = par->hd.mem.buffer;
	par->hd.fb0                = NEXUS_Surface_Create(&createSettings);
	if(par->hd.fb0 == NULL) goto err_surface;
	printk("hdfb%d-0::%dx%d::s:%d::b:%p::o:%p::%p\n", info->node,
		createSettings.width, createSettings.height,
		createSettings.pitch, createSettings.pMemory,
		createSettings.pMemory, par->hd.fb0);

	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 4;
	createSettings.pMemory     = par->hd.mem.buffer + (videoFormatInfo.height * videoFormatInfo.width * 4);
	par->hd.fb1                = NEXUS_Surface_Create(&createSettings);
	if(par->hd.fb1 == NULL) goto err_surface;
	printk("hdfb%d-1::%dx%d::s:%d::b:%p::o:%p::%p\n", info->node,
		createSettings.width, createSettings.height,
		createSettings.pitch, createSettings.pMemory,
		createSettings.pMemory, par->hd.fb1);

	if (par->hd.display) {
		NEXUS_Display_GetGraphicsSettings(par->hd.display, &graphicsSettings);
		graphicsSettings.enabled           = true;
		graphicsSettings.clip.width        = videoFormatInfo.width;
		graphicsSettings.clip.height       = videoFormatInfo.height;
		graphicsSettings.sourceBlendFactor = NEXUS_CompositorBlendFactor_eOne;
		graphicsSettings.destBlendFactor   = NEXUS_CompositorBlendFactor_eZero;
		graphicsSettings.constantAlpha     = 0xFF;
		NEXUS_Display_SetGraphicsSettings(par->hd.display, &graphicsSettings);
	}

	info->fix.smem_start  = (unsigned long)NEXUS_AddrToOffset(par->hd.mem.buffer);
	info->fix.line_length = par->hd.mem.pitch / 2;
	info->fix.smem_len    = createSettings.height * par->hd.mem.pitch;
	info->fix.visual      = FB_VISUAL_TRUECOLOR;

	info->var.xres         = createSettings.width;
	info->var.yres         = createSettings.height;
	info->var.xres_virtual = createSettings.width;
	info->var.yres_virtual = createSettings.height;

	info->screen_base = (char __force __iomem *)par->hd.mem.buffer;
	info->screen_size = info->fix.smem_len / 2;

#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	rc = NEXUS_HdmiOutput_GetStatus(hdmiOutput, &hdmiStatus);
	if (rc == NEXUS_SUCCESS && hdmiStatus.connected) {
		par->sd.display = NULL;
		par->sd.framebuffer = NULL;
		goto done;
	}

	NEXUS_Display_GetDefaultSettings(&displaySettings);
	displaySettings.format = NEXUS_VideoFormat_ePal;
	par->sd.display = NEXUS_Display_Open(1, &displaySettings);
	if(!par->sd.display) goto err_display;
	rc = NEXUS_Display_AddOutput(par->sd.display, NEXUS_CompositeOutput_GetConnector(platformConfig.outputs.composite[0]));
	if(rc!=NEXUS_SUCCESS) goto err_output;

	NEXUS_VideoFormat_GetInfo(displaySettings.format, &videoFormatInfo);
	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width * 2;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 2 * 4;
	createSettings.heap        = NEXUS_Platform_GetFramebufferHeap(0);
	par->sd.framebuffer = NEXUS_Surface_Create(&createSettings);
	if(par->sd.framebuffer == NULL) goto err_graphics;
	rc = NEXUS_Surface_GetMemory(par->sd.framebuffer, &(par->sd.mem));
	if(rc!=NEXUS_SUCCESS) goto err_surface;

	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 4;
	createSettings.pMemory     = par->sd.mem.buffer;
	par->sd.fb0 = NEXUS_Surface_Create(&createSettings);
	if(par->sd.fb0 == NULL) goto err_surface;
	printk("sdfb%d-0::%dx%d::s:%d::b:%p::o:%p::%p\n", info->node,
		createSettings.width, createSettings.height,
		createSettings.pitch, createSettings.pMemory,
		createSettings.pMemory, par->sd.fb0);

	NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
	createSettings.pixelFormat = NEXUS_PixelFormat_eX8_B8_G8_R8;
	createSettings.width       = videoFormatInfo.width;
	createSettings.height      = videoFormatInfo.height;
	createSettings.pitch       = videoFormatInfo.width * 4;
	createSettings.pMemory     = par->sd.mem.buffer + (videoFormatInfo.height * videoFormatInfo.width * 4);
	par->sd.fb1 = NEXUS_Surface_Create(&createSettings);
	if(par->sd.fb1 == NULL) goto err_surface;
	printk("sdfb%d-1::%dx%d::s:%d::b:%p::o:%p::%p\n", info->node,
		createSettings.width, createSettings.height,
		createSettings.pitch, createSettings.pMemory,
		createSettings.pMemory, par->sd.fb1);

	NEXUS_Display_GetGraphicsSettings(par->sd.display, &graphicsSettings);
	graphicsSettings.enabled           = true;
	graphicsSettings.clip.width        = videoFormatInfo.width;
	graphicsSettings.clip.height       = videoFormatInfo.height;
	graphicsSettings.sourceBlendFactor = NEXUS_CompositorBlendFactor_eOne;
	graphicsSettings.destBlendFactor   = NEXUS_CompositorBlendFactor_eZero;
	graphicsSettings.constantAlpha     = 0xFF;
	NEXUS_Display_SetGraphicsSettings(par->sd.display, &graphicsSettings);
#endif

done:
	printk("fb%d::%dx%d::v:%dx%d::base:%p::sz:%lu::%u:%u/%zu\n",
		info->node,
		info->var.xres, info->var.yres,
		info->var.xres_virtual, info->var.yres_virtual,
		(void *)info->screen_base, info->screen_size,
		info->fix.line_length, info->fix.smem_len,
		par->hd.mem.bufferSize);
	par->ref_cnt++;
	return 0;

err_surface:
	printk("fb%d: err_surface - FAILED\n", info->node);
	if (par->hd.fb0) NEXUS_Surface_Destroy(par->hd.fb0);
	if (par->hd.fb1) NEXUS_Surface_Destroy(par->hd.fb1);
	if (par->hd.framebuffer) NEXUS_Surface_Destroy(par->hd.framebuffer);
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.fb0) NEXUS_Surface_Destroy(par->sd.fb0);
	if (par->sd.fb1) NEXUS_Surface_Destroy(par->sd.fb1);
	if (par->sd.framebuffer) NEXUS_Surface_Destroy(par->sd.framebuffer);
#endif
err_graphics:
	printk("fb%d: err_graphics - FAILED\n", info->node);
	if (par->hd.display) NEXUS_Display_RemoveAllOutputs(par->hd.display);
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.display) NEXUS_Display_RemoveAllOutputs(par->sd.display);
#endif
err_output:
	printk("fb%d: err_output - FAILED\n", info->node);
	if (par->hd.display) NEXUS_Display_Close(par->hd.display);
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.display) NEXUS_Display_Close(par->sd.display);
#endif
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
	NEXUS_Graphics2D_Close(par->gfx);
	if (par->hd.display) NEXUS_Display_Close(par->hd.display);
	if (par->hd.fb0) NEXUS_Surface_Destroy(par->hd.fb0);
	if (par->hd.fb1) NEXUS_Surface_Destroy(par->hd.fb1);
	if (par->hd.framebuffer) NEXUS_Surface_Destroy(par->hd.framebuffer);
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.display) NEXUS_Display_Close(par->sd.display);
	if (par->sd.fb0) NEXUS_Surface_Destroy(par->sd.fb0);
	if (par->sd.fb1) NEXUS_Surface_Destroy(par->sd.fb1);
	if (par->sd.framebuffer) NEXUS_Surface_Destroy(par->sd.framebuffer);
#endif
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

	if (par->hd.display) {
		NEXUS_Display_GetGraphicsSettings(par->hd.display, &graphicsSettings);
		graphicsSettings.visible = blank_mode==FB_BLANK_UNBLANK?true:false;
		NEXUS_Display_SetGraphicsSettings(par->hd.display, &graphicsSettings);
	}
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.display) {
		NEXUS_Display_GetGraphicsSettings(par->sd.display, &graphicsSettings);
		graphicsSettings.visible = blank_mode==FB_BLANK_UNBLANK?true:false;
		NEXUS_Display_SetGraphicsSettings(par->sd.display, &graphicsSettings);
	}
#endif
	return 0;
}

void bcmnexusfb_fillrect(struct fb_info *p, const struct fb_fillrect *region)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)p->par;
	NEXUS_Graphics2DFillSettings fillSettings;

	NEXUS_Graphics2D_GetDefaultFillSettings(&fillSettings);
	fillSettings.surface = par->hd.framebuffer;
	fillSettings.rect.x = region->dx;
	fillSettings.rect.y = region->dy;
	fillSettings.rect.width = region->width;
	fillSettings.rect.height = region->height;
	fillSettings.color = region->color;

	if (debug_verbose) printk("fb%d: fillrect %d,%d %d,%d\n",
		p->node, region->dx, region->dy, region->width, region->height);

	NEXUS_Graphics2D_Fill(par->gfx, &fillSettings);
}

void bcmnexusfb_copyarea(struct fb_info *p, const struct fb_copyarea *area)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)p->par;
	NEXUS_Graphics2DBlitSettings blitSettings;

	NEXUS_Graphics2D_GetDefaultBlitSettings(&blitSettings);

	blitSettings.source.surface = par->hd.framebuffer;
	blitSettings.source.rect.x = area->sx;
	blitSettings.source.rect.y = area->sy;
	blitSettings.source.rect.width = area->width;
	blitSettings.source.rect.height = area->height;

	blitSettings.output.surface = par->hd.framebuffer;
	blitSettings.output.rect.x = area->dx;
	blitSettings.output.rect.y = area->dy;
	blitSettings.output.rect.width = area->width;
	blitSettings.output.rect.height = area->height;

	blitSettings.colorOp = NEXUS_BlitColorOp_eCopySource;
	blitSettings.alphaOp = NEXUS_BlitAlphaOp_eCopySource;

	if (debug_verbose) printk("fb%d: copyarea %d,%d %d,%d\n",
		p->node, area->dx, area->dy, area->width, area->height);

	NEXUS_Graphics2D_Blit(par->gfx, &blitSettings);
}

void bcmnexusfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	printk("fb%d: imageblit - NOT SUPPORTED \n", info->node);
}

int bcmnexusfb_sync(struct fb_info *info)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;

	if (debug_verbose) printk("fb%d: sync\n", info->node);

	NEXUS_Surface_Flush(par->hd.framebuffer);
#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.framebuffer) NEXUS_Surface_Flush(par->sd.framebuffer);
#endif
	return 0;
}

int bcmnexusfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (info->flags & FBINFO_MISC_USEREVENT) {
		if (debug_verbose) printk("fb%d: check-var %d::%d::%d\n",
			info->node, var->yres_virtual, var->yoffset,
			var->bits_per_pixel);
	}

	return 0;
}

int bcmnexusfb_set_par(struct fb_info *info)
{
	struct bcmnexusfb_par *par = (struct bcmnexusfb_par*)info->par;

	if (!(info->flags & FBINFO_MISC_USEREVENT)) return 0;

	if (debug_verbose) printk("fb%d: set-par:%d >> set-fb:%p\n",
		info->node, info->var.yoffset,
		!info->var.yoffset ? par->hd.fb0 : par->hd.fb1);

	if (par->hd.display) {
		NEXUS_Display_SetGraphicsFramebuffer(
			par->hd.display,
			!info->var.yoffset ? par->hd.fb0 : par->hd.fb1);
	}

#if NEXUS_NUM_DISPLAYS > 1 && NEXUS_NUM_COMPOSITE_OUTPUTS
	if (par->sd.display) {
		NEXUS_Error rc;
		int timeout = 10;

		NEXUS_GraphicsSettings graphicsSettings;
		NEXUS_Graphics2DBlitSettings blitSettings;
		NEXUS_Graphics2D_GetDefaultBlitSettings(&blitSettings);
		if (!info->var.yoffset) {
			blitSettings.source.surface = par->hd.fb0;
			blitSettings.output.surface = par->sd.fb0;
		} else {
			blitSettings.source.surface = par->hd.fb1;
			blitSettings.output.surface = par->sd.fb1;
		}
		if (par->hd.display) {
			NEXUS_Display_GetGraphicsSettings(par->hd.display, &graphicsSettings);
			blitSettings.source.rect.width = graphicsSettings.clip.width;
			blitSettings.source.rect.height = graphicsSettings.clip.height;
		} else {
			blitSettings.source.rect.width = 1920;
			blitSettings.source.rect.height = 1080;
		}
		NEXUS_Display_GetGraphicsSettings(par->sd.display, &graphicsSettings);
		blitSettings.output.rect.width = graphicsSettings.clip.width;
		blitSettings.output.rect.height = graphicsSettings.clip.height;
		blitSettings.colorOp = NEXUS_BlitColorOp_eCopySource;
		blitSettings.alphaOp = NEXUS_BlitAlphaOp_eCopySource;
		rc = NEXUS_Graphics2D_Blit(par->gfx, &blitSettings);
		do {
			rc = NEXUS_Graphics2D_Checkpoint(par->gfx, NULL);
			if (rc != NEXUS_GRAPHICS2D_BUSY) break;
			msleep(100);
		} while (timeout--);

		NEXUS_Display_SetGraphicsFramebuffer(
			par->sd.display,
			!info->var.yoffset ? par->sd.fb0: par->sd.fb1);
	}
#endif

	return 0;
}

static struct fb_ops bcmnexusfb_ops = {
	.owner			= THIS_MODULE,
	.fb_open		= bcmnexusfb_open,
	.fb_release		= bcmnexusfb_release,
	.fb_setcolreg		= bcmnexusfb_setcolreg,
	.fb_blank		= bcmnexusfb_blank,
	.fb_fillrect		= bcmnexusfb_fillrect,
	.fb_copyarea		= bcmnexusfb_copyarea,
	.fb_imageblit		= bcmnexusfb_imageblit,
	.fb_sync		= bcmnexusfb_sync,
	.fb_check_var		= bcmnexusfb_check_var,
	.fb_set_par		= bcmnexusfb_set_par,
};

static int __init bcmnexusfb_init(void)
{
	struct bcmnexusfb_par *par;
	struct fb_info *info;

	/*
	 * Dynamically allocate info and par
	*/
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->par = kzalloc(sizeof(struct bcmnexusfb_par), GFP_KERNEL);
	if (!info->par) {
		kfree(info);
		return -ENOMEM;
	}

	bcmfb = info;
	par = info->par;
	par->ref_cnt = 0;

	info->screen_base = NULL;

	info->fbops = &bcmnexusfb_ops;
	info->var = bcmnexusfb_var;
	info->fix = bcmnexusfb_fix;

	info->pseudo_palette = par->pseudo_palette;
	info->flags = FBINFO_DEFAULT |
	    FBINFO_HWACCEL_COPYAREA |
	    FBINFO_HWACCEL_FILLRECT |
	    FBINFO_HWACCEL_IMAGEBLIT;

	fb_alloc_cmap(&bcmfb->cmap, 256, 0);

	if (register_framebuffer(bcmfb) < 0)
		return -EINVAL;

	printk(KERN_INFO "fb%d: %s frame buffer device\n",
		bcmfb->node, bcmfb->fix.id);

	return 0;
}

static void __exit bcmnexusfb_exit(void)
{
	if (bcmfb != NULL) {
		unregister_framebuffer(bcmfb);
		fb_dealloc_cmap(&bcmfb->cmap);
		kfree(bcmfb->par);
		kfree(bcmfb);
		bcmfb = NULL;
	}

	return;
}

module_init(bcmnexusfb_init);
module_exit(bcmnexusfb_exit);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Broadcom Limited");
MODULE_DESCRIPTION("framebuffer over nexus");
