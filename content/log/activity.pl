#!/usr/bin/perl



my $count=0;
my $ls = "";
my @plist = ();


@infiles = </home/pantheon/avprod/log/3362.log>;

open (OF, ">/home/pantheon/avprod/log/activity.csv");

foreach $file (@infiles)
{
    open (LF, $file);
    while (<LF>)
    {
	$line = $_;
	next if ($line =~ m/Notify/);
	next if ($line =~ m/Brazen/);
	next if ($line =~ m/Neongrey/);
	next if ($line =~ m/Ninjadyne/);
	next if ($line =~ m/Tzajai/);
	next if ($line =~ m/Vaialos/);
	next if ($line =~ m/Dovolente/);
	next if ($line =~ m/Rveyelhi/);
	next if ($line =~ m/Transmitt/);
	next if ($line =~ m/Jolinn/);
	next if ($line =~ m/Arkhural/);
	next if ($line =~ m/Bayyal/);
	next if ($line =~ m/Serachel/);
	$time = substr($line,0,index($line,"::")-1);
	next if ($ls eq $time);
	if ($line =~ m/has connected\.$/)
	{
	    $count++;
	    print $count . "   " . $line;
	    $time = substr($line,0,index($line,"::")-1);
	    print OF chr(34) . $time . chr(34) . ",";
	    print OF chr(34) . $count . chr(34) . ",\n";
	    $pname = substr($line,index($line,":: "),index($line,"@")-index($line,":: "));
	    push @plist, $pname;
	};
#	if ($line =~ m/reconnected\.$/)
#	{
#	    $count++;
#	    print $count . "   " . $line;
#	    $time = substr($line,0,index($line,"::")-1);
#	    print OF chr(34) . $time . chr(34) . ",";
#	    print OF chr(34) . $count . chr(34) . ",\n";
#	    $pname = substr($line,index($line,":: "),index($line,"@")-index($line,":: "));
#	    push @plist, $pname;
#	};
	if ($line =~ m/new player\.$/)
	{
	    $count++;
	    print $count . "   " . $line;
	    $time = substr($line,0,index($line,"::")-1);
	    print OF chr(34) . $time . chr(34) . ",";
	    print OF chr(34) . $count . chr(34) . ",\n";
	    $pname = substr($line,index($line,":: "),index($line,"@")-index($line,":: "));
	    push @plist, $pname;
	};

	if ($line =~ m/has quit\.$/)
	{
	    $count--;
	    print $count . "   " . $line;
	    $time = substr($line,0,index($line,"::")-1);
	    print OF chr(34) . $time . chr(34) . ",";
	    print OF chr(34) . $count . chr(34) . ",\n";
	    $pname = substr($line,index($line,":: "),index($line,"@")-index($line,":: "));
	    for my $i (0 .. $#plist)
	    {
		delete @plist[$i] if ($_ =~ $pname);
	    };
		
	};
#	if ($line =~ m/Closing link to/)
#	{
#	    my $oc = $count;
#	    $pname = substr($line,index($line,":: "),index($line,"@")-index($line,":: "));
#	    for my $i (0 .. $#plist)
#	    {
#		if ($_ =~ $pname)
#		{
#	    	    $count--;
#		    print $count . "   " . $line;
#		    $time = substr($line,0,index($line,"::")-1);
#		    print OF chr(34) . $time . chr(34) . ",";
#		    print OF chr(34) . $count . chr(34) . ",\n";
#		};
#	    };
#	    next if ($oc == $count);
#	    for my $i (0 .. $#plist)
#	    {
#		delete @plist[$i] if ($_ =~ $pname);
#	    };
#	};
	$ls = $time;
    };
    close(LF);
    $count=0;
};
close (OF);
