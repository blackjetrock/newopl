#!/usr/bin/tclsh


set ::iy 100001

proc next {} {
    
    set ::iy [expr $::iy*125]
    set ::iy [expr $::iy % 2796203]
    set r [expr $::iy / 2796203.0]
    return $r
}


set i 0

while { $i < 10 } {
    puts "$i [next]"
    incr i 1
}

	
