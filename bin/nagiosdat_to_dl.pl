#!/usr/bin/env perl

# NOTE: This is here purely as an example.
# It happens to work on our outdated Nagios config,
# but I make no gaurantees for yours.
#
# Please, just use this as an example.

use strict;
use warnings;
use File::Slurp;
use LWP::Simple;

my $ip              = shift;
my $nagios_dat_file = shift;

# list of nagios services to look for
my $target_data = {
    'www04'   => 0,
    'www05'   => 1,
    'www06'   => 2,
    'www07'   => 3,
    'www08'   => 4,
    'www09'   => 5,
    'admin01' => 6,
    'admin02' => 7,
    'db03'    => 8,
    'db04'    => 8,
    'cpu05'   => 9,
};

# nagios states mapped to color:
my $nagios_states = {
    0 => '000000',    # good
    1 => '0000ff',    # pending
    2 => 'ff0000',    # critical
    4 => 'ffff00',    # warning
    8 => '0000ff',    # unknown
};

# offset of pixel to show each service in a column
# if load is at x0, y0 then memory will be at x1,y0 etc...
my $light_map = {
    LOAD         => 0,
    'LOAD_12-15' => 0,
    'LOAD_15-18' => 0,
    MEMORY       => 1,
    'DISK: /'    => 2
};

# parse the file and build a perl data structure
my $nagios_data = parse_dat_nagios($nagios_dat_file);

#check desired services
foreach my $service_data ( @{ $nagios_data->{servicestatus} } ) {
    next
        unless ( defined $target_data->{ $service_data->{host_name} }
        && defined $light_map->{ $service_data->{service_description} } );
    my $x   = $target_data->{ $service_data->{host_name} };
    my $y   = $light_map->{ $service_data->{service_description} };
    my $h   = $nagios_states->{ $service_data->{current_state} };
    # This predates the dl util and could make use of the frame command
    # to set the whole desk in one http post command.
    my $url = 'http://' . $ip . "/pixel?x=$x&y=$y&h=$h";
    get $url;
}

### support functions

sub trim {
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

sub parse_dat_nagios {
    my $infile = shift;
    my @data   = read_file( $infile, chomp => 1 );
    my $out    = {};
    my $section;
    my $temp = {};
    foreach my $line (@data) {
        next if ( substr( $line, 0, 1 ) eq '#'
            || length($line) == 0 );

        if ( index( $line, '{' ) >= 0 ) {
            ($section) = split( ' ', $line );
            next;
        }
        elsif ( index( $line, '}' ) >= 0 ) {
            push @{ $out->{$section} }, $temp;
            $temp = {};
        }
        else {
            my ( $tag, $value ) = split( /=/, $line, 2 );
            $temp->{ trim($tag) } = trim($value);
        }

    }

    return $out;
}
