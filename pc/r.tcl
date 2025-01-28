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

set ::s 1

proc pr {} {

    repeat 48 times
    
    digits[0] + lsbitof(upperdigit[0] xor lowdigit[6])
    
    top digit xor 6
    get ls bit
    
}

set ::N "000000000001"

proc daa {x} {
    if { $x > 9 } {
	set x [expr $x - 10]
    }

    return $x
}

proc rnd {x} {

    set i 0

    while { $i < 12 } {
	set d($i) [string range $x $i $i] 
    }
    
    puts "$d(0) $d(6)"

    set i 0
    while {$i <48 } {
	set digits0 [expr $d0 + (($d0 ^ $d6) & 1)]
	puts $nd0

	
	incr i 1
    }
}


rnd $::N

