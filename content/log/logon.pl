#!/usr/bin/perl

$files = `ls 31*.log`;
@files = split /\s+/, $files;

foreach $file (@files)
{
	open INPUT, $file;
	while (<INPUT>)
	{
		$line = $_;
		if (($line =~ /Azgrah/ || $line =~ /Ixjacht/ ))
		{
			print $line;
		}
	}
	close INPUT;
}
