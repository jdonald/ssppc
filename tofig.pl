#!/usr/bin/perl
#use warnings;

#params
#$maxx = 12600;
#$maxy = 9600;
$maxx = 9600;
$maxy = 12600;
$res = 1200;

sub usage () {
    print("usage: tofig.pl [-a <area ratio>] [-f <fontsize>] [-s <nskip>] <file>\n");
    print("<area ratio> -- approx page occupancy by figure (default 0.95)\n");
    print("<fontsize>   -- font size to be used (default 12)\n");
    print("<nskip>      -- no. of entries to be skipped in input (default 0)\n");
    print("<file>       -- input file\n");
    exit(1);
}

&usage() if (@ARGV > 7 || !@ARGV % 2 || ! -f $ARGV[@ARGV-1]);

$occupancy = 0.95;
$fontsize = 12;
$nskip = 0;

for($i=0; $i < @ARGV-1; $i+=2) {
	if ($ARGV[$i] eq "-a") {
		$occupancy=$ARGV[$i+1];
		next;
	}

	if ($ARGV[$i] eq "-f") {
		$fontsize=$ARGV[$i+1];
		next;
	}

	if ($ARGV[$i] eq "-s") {
		$nskip=$ARGV[$i+1];
		next;
	}

	&usage();
	exit(1);
}

open (FILE, "<$ARGV[@ARGV-1]") || die("error: file $ARGV[@ARGV-1] could not be opened\n");
$maxfig = 0;
$minfig = -1;
while (<FILE>) {
	last if (/FIG starts/);
}
$pos=tell(FILE);
$j=0; 
while (<FILE>) {
	last if (/FIG ends/);
	next if (/[a-zA-Z]|^\s*$/);
	if ($j < $nskip) {
		$j++;
		next;
	}
	chomp;
	@nums = split(/\s+/);
	for ($i=0; $i < @nums; $i++) {
		die("error: negative co-ordinate found\n") if ($nums[$i] < 0);
		$maxfig = $nums[$i] if ($nums[$i] > $maxfig);
		$minfig = $nums[$i] if ($nums[$i] < $minfig || $minfig == -1);
	}
}

$maxfig-=$minfig;
$delta = int(25/sqrt($occupancy));
$scale = (($maxx < $maxy)?$maxx:$maxy) / $maxfig * sqrt($occupancy);
$xorig = ($maxx-$maxfig*$scale)/2;
$yorig = ($maxy-$maxfig*$scale)/2;

print "#FIG 3.1\nPortrait\nCenter\nInches\n$res 2\n";
seek(FILE, $pos, 0);

$j=0;
while (<FILE>) {
	last if (/FIG ends/);
	if ($j < $nskip * 2) {
		$j++;
		next;
	}	
	chomp;
	@coords = split(/\s+/);
	next if ($#coords == -1);
	@coords = map($_-$minfig, @coords);
	@coords = map($_*$scale, @coords);
	for ($i=0; $i < @coords; $i++) {
		if ($i % 2) {
			$coords[$i] = int($maxy - $coords[$i] - $yorig);
		}	
		else {
			$coords[$i] = int($coords[$i] + $xorig);
		}	
	}
	printf("2 2 0 1 -1 7 0 0 -1 0.000 0 0 0 0 0 %d\n", int (@coords)/2);
	print "\t@coords\n";
	$xpos = $coords[0] + $delta;
	$ypos = $coords[1] - $delta;
	$name = <FILE>;
	chomp($name);
	print "4 0 -1 0 0 0 $fontsize 0.0000000 4 -1 -1 $xpos $ypos $name\\001\n";
}
close(FILE);
