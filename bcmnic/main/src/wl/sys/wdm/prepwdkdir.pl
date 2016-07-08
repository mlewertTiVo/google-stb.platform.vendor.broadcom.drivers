#!/usr/bin/env perl
## 
## Script to populate windows wdk buildspace from workspace
## 
## $Id: prepwdkdir.pl,v 1.1 2008-04-15 05:42:49 prakashd Exp $

use File::Basename;
use File::Compare;
use File::Copy;
use Getopt::Long;

my $prog = basename($0);

my %opt;
GetOptions(\%opt,
    qw(src_files=s@ install generated=s@ output=s blddir=s treebase=s verbose));

my %seen;
my @src_files = grep { !$seen{$_}++ } map { split ' ' } @{$opt{src_files}};
my %generated = map { $_ => 1 } map { split ' ' } @{$opt{generated}};
my $blddir = $opt{blddir};
my $output = $opt{output};
my $treebase = $opt{treebase};

unless (@src_files && $output && $blddir && $treebase) {
   select STDERR;
   print "$prog: Error: Missing required cmd line arguments\n";
   print "Usage: \n";
   print " $prog -src_files '<wl-src-files>' -treebase <tree-base>\n";
   print " \t-output <sources-file-name> -blddir <wdm-build-dir>\n";
   exit(1);
}

my @update_wlsrcs;
my @srclist;
@src_files = sort { basename($a) cmp basename($b) } @src_files;
for my $wlsrc ( map { "$treebase$_" } @src_files ) {
    push(@srclist, $wlsrc);
    my $wlbase = basename($wlsrc);
    my $wltgt = "$blddir/$wlbase";
    if (! $generated{$wlbase}) {
	push(@update_wlsrcs, $wlsrc)
	    if ! -e $wltgt || -M $wlsrc < -M $wltgt;
    }
}

open(SOURCES, ">>", $output) || die "$prog: Error: $output: $!\n";
print SOURCES "SOURCES  = \\\n";
for (@srclist) {
    print SOURCES "           ", basename($_);
    print SOURCES " \\" unless $_ eq $srclist[-1];
    print SOURCES "\n";
}
printf SOURCES "\n# End of %s sources file\n", $ENV{WINOS} || 'windows';
close(SOURCES);

if ($opt{install} && @update_wlsrcs) {
    for (@update_wlsrcs) {
	my $to = join('/', $blddir, basename($_));
	next if -f $to && -f $_ && (-s $_ == -s $to) && !compare($_, $to);
	print STDERR "+ cp -p $_ $blddir\n" if $opt{verbose};
	File::Copy::syscopy($_, $blddir) || die "$prog: Error: $output: $!\n";
    }
}
