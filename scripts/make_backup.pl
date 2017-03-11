#!/usr/bin/perl -w

$path_staging = "/home/pantheon/backups/staging";
$path_dev_areas = "/home/pantheon/avdev/area";
$path_prod = "/home/pantheon/avprod";
$path_pantheon = "/home/pantheon";
$path_src = "/home/chadim/svn/trunk";

print "Enter path (including extensionless filename) of backup file:\n";
$backup_name = <STDIN>;
chomp $backup_name;

print "Include database in backup [Y for yes, anything else for no]?\n";
$include_database = <STDIN>;
chomp $include_database;

$should_backup_db = 0;
if ($include_database eq "y" or $include_database eq "Y")
{
    $should_backup_db = 1;
}

print "Preparing staging area\n";
system("rm -rf $path_staging");
system("mkdir $path_staging");
print "Backing up website\n";
system("tar -czf $path_staging/website.tar.gz $path_pantheon/public_html");
print "Backing up miscellaneous items from $path_pantheon\n";
system("tar -czf $path_staging/misc.tar.gz $path_pantheon/ban.txt $path_pantheon/notify $path_pantheon/*.pl $path_pantheon/starter");
print "Backing up development areas\n";
system("tar -czf $path_staging/dev_areas.tar.gz $path_dev_areas/*.lst $path_dev_areas/*.are $path_dev_areas/notes.not");
print "Backing up production (everything)\n";
system("tar -czf $path_staging/prod.tar.gz $path_prod/*");
print "Backing up source\n";
system("rm -rf $path_src/*.o");
system("tar -czf $path_staging/source.tar.gz $path_src/*");

if ($should_backup_db == 1)
{
    print "Backing up database\n";
    system("mysqldump -u pantheon --password=BuFF3L1ve5! --all-databases > $path_staging/db.sql");
    print "Combining backups\n";
}

print "Packaging everything together\n";
system("tar -czf $backup_name.tar.gz $path_staging");
print "Finished: $backup_name.tar.gz created\n";
