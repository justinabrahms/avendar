#!/usr/bin/perl

my $count=0;
my @plist = ();
my $found;
my $output;
my $ff=1;
my $i;

@infiles = </home/pantheon/avprod/log/33*.log>;
open (OF, ">/home/pantheon/avprod/log/activity.csv");

foreach $file (@infiles)
{
    open (LF, $file);
    while (<LF>)
    {
	$line = $_;
	if ($ff)
	{
	    if ($line =~ m/ready to rock on port 9999\.$/)
	    {
		$ff = 0;
	    }
	    else
	    {
		next;
	    };
	};
	next if (immname($line));
	$time = substr($line,0,index($line,"::")-1);
	if ($line =~ m/has connected\.$/)
	{
	    $pname = substr($line,index($line,":: ")+3,index($line,"@")-(index($line,":: ")+3));
	    $found = -1;
	    for $i (0 .. $#plist)
	    {
		if ($plist[$i] eq $pname)
		{
		    $found = $i;
		    last;
		};
	    };
	    if ($found > -1){}
	    else
	    {
	    	$count++;
	    	push @plist, $pname;
		$time = substr($line,0,index($line," ::"));
		$output = chr(34) . $time . chr(34) . "," . chr(34) . $count . chr(34) . "," . chr(34) . "@plist" . chr(34);
	 	print OF $output . "\n";
#	 	print $output . "@plist" . "\n";
	    };
	};
	if ($line =~ m/reconnected\.$/)
	{
	    $pname = substr($line,index($line,":: ")+3,index($line,"@")-(index($line,":: ")+3));
	    $found = -1;
	    for $i (0 .. $#plist)
	    {
		if ($plist[$i] eq $pname)
		{
		    $found = $i;
		    last;
		};
	    };
	    if ($found > -1){}
	    else
	    {
	    	$count++;
	    	push @plist, $pname;
		$time = substr($line,0,index($line,"::")-1);
		$output = chr(34) . $time . chr(34) . "," . chr(34) . $count . chr(34) . "," . chr(34) . "@plist" . chr(34);
	 	print OF $output . "\n";
#	 	print $output . "@plist" . "\n";
	    };
	};
	if ($line =~ m/new player\.$/)
	{
	    $pname = substr($line,index($line,":: ")+3,index($line,"@")-(index($line,":: ")+3));
	    $found = -1;
	    for $i (0 .. $#plist)
	    {
		if ($plist[$i] eq $pname)
		{
		    $found = $i;
		    last;
		};
	    };
	    if ($found>-1){}
	    else
	    {
	        $count++;
	    	push @plist, $pname;
	    	$time = substr($line,0,index($line,"::")-1);
		$output = chr(34) . $time . chr(34) . "," . chr(34) . $count . chr(34) . "," . chr(34) . "@plist" . chr(34);
	 	print OF $output . "\n";
#	 	print $output . "@plist" . "\n";
	    };
	};

	if ($line =~ m/has quit\.$/)
	{
	    if ($line =~ m/\@/) {
		$pname = substr($line,index($line,":: ")+3,index($line,"@")-(index($line,":: ")+3));
	    } else {
		$pname = substr($line,index($line,":: ")+3,$#line-(index($line,"has quit. ")+3));
	    }
	    $found = -1;
	    for $i (0 .. $#plist)
	    {
		if ($plist[$i] eq $pname)
		{
		    $found = $i;
		    last;
		};
	    };
	    if ($found > -1)
	    {
		$count--;
		if ($found == 0) {
		    shift(@plist);
		} elsif ($found == $#plist) {
		    pop(@plist);
		} else {
		    @plist = @plist[0..$found-1,$found+1..$#plist];
#		    splice(@plist,$found-1,1);
		};
		$time = substr($line,0,index($line,"::")-1);
		$output = chr(34) . $time . chr(34) . "," . chr(34) . $count . chr(34) . "," . chr(34) . "@plist" . chr(34);
	 	print OF $output . "\n";
#	 	print $output . "@plist" . "\n";
	    };
	};
	if ($line =~ m/Closing link to/)
	{
	    my $oc = $count;
	    $pname = substr($line,index($line,"to ")+3,$#line-1);
	    $found = -1;
	    for $i (0 .. $#plist)
	    {
		if ($plist[$i] eq $pname)
		{
		    $found = $i;
		    last;
		};
	    };
	    if ($found > -1)
	    {
	        $count--;
		if ($found == 0) {
		    shift(@plist);
		} elsif ($found == $#plist) {
		    pop(@plist);
		} else {
		    @plist = @plist[0..$found-1,$found+1..$#plist];
#		    splice(@plist,$found-1,1);
		};
		$time = substr($line,0,index($line,"::")-1);
		$output = chr(34) . $time . chr(34) . "," . chr(34) . $count . chr(34) . "," . chr(34) . "@plist" . chr(34);
	 	print OF $output . "\n";
#	 	print $output . "@plist" . "\n";
	    };
	    next if ($oc == $count);
	};
    };
    close(LF);
    pop @plist while (@plist);
    $count=0;
};
close (OF);

sub immname()
{
    my $l = $_[0];
    if (($l =~ m/Sock.sinaddr/)
    || ($l =~ m/EOF/)
    || ($l =~ m/input spamming/)
    || ($l =~ m/Notify/)
    || ($l =~ m/Brazen/)
    || ($l =~ m/Sardonix/)
    || ($l =~ m/Neongrey/)
    || ($l =~ m/Ninjadyne/)
    || ($l =~ m/Tzajai/)
    || ($l =~ m/Vaialos/)
    || ($l =~ m/Dovolente/)
    || ($l =~ m/Rveyelhi/)
    || ($l =~ m/Transmitt/)
    || ($l =~ m/Jolinn/)
    || ($l =~ m/Arkhural/)
    || ($l =~ m/Bayyal/)
    || ($l =~ m/Serachel/))
    {
	return 1;
    };
    return 0;
};
