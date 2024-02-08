#!/usr/bin/tclsh

# Convert a text file with ascii hex in it to a bin file with just
# the ascii hex converted to binary

set fn [lindex $argv 0]
set ofn $fn.bin

puts "Opening $fn"
puts "Output file '$ofn'"

set f  [open $fn]
set of [open $ofn wb]
fconfigure $of -translation binary
set txt [read $f]

set hexdata ""

foreach line [split $txt "\n"] {
    puts $line

    # Take any ascii hex from the front of each line
    if { [regexp -- "(\[A-Za-z0-9 \]+)  " [string trim $line " "] all hex rest] } {
	set hexdata $hexdata[string map {" " ""} $hex]
	puts "-$hex-"
    } else {
	if { [regexp -- "(\[A-Za-z0-9 \]+)$" [string trim $line " "] all hex rest] } {
	    set hexdata $hexdata[string map {" " ""} $hex]
	    puts "-$hex-2"
	}
    }
}

puts $hexdata

set bin [binary format H* $hexdata]

puts -nonewline $of $bin

close $of
close $f

