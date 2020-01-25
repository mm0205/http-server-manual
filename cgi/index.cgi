#!/usr/bin/perl

use strict;
use warnings;
use CGI;
use JSON;

my $cgi = CGI->new();
my $json  = JSON->new->utf8(0);

my $post_data = "";
read (STDIN, $post_data, $ENV{'CONTENT_LENGTH'});
my $posted_json = $json->decode($post_data);
my $encoded_json = $json->encode($posted_json);
print <<"END";
これは Perl スクリプトから出力された文字列です。
$encoded_json
END

