#!/usr/bin/perl -w


my $argc = @ARGV;

my $msg = "";

`dmesg -c`;
if( $ARGV[0] eq "stop" )
{
	`pkill -9 ukmem_uspace`;
	`rmmod igb`;
	`rmmod ukmem`;
	`insmod /lib/modules/2.6.35/build/drivers/net/igb/igb.ko`;
}
elsif( $ARGV[0] eq "start" )
{
	if($argc != 2)
	{
		print "Usage:tool start queue_num=<n> |stop\n";
		exit ;
	}
	
	`rmmod igb`;
	`rmmod ukmem`;
	`insmod ukmem.ko`;
	`insmod igb.ko $ARGV[1]`
}
else
{
	print "Usage:tool start queue_num=<n> |stop\n";
}

$msg = `dmesg -c`;

print "$msg\n";
