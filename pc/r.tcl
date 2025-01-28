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


set ::N "00000000000001"
set ::N "22222222222222"
set ::N "12345678901234"


proc daa {x} {
    if { $x > 9 } {
	set x [expr $x - 10]
    }

    return $x
}

proc rnd {x} {
    #puts "n:'$::N' len:[string length $::N]"

    set i 0
    while { $i < 14 } {
	set d($i) [string range $::N $i $i]
	incr i 1
    }
    
    set i 0
    while {$i < 48 } {
	set d(0) [expr $d(0) + (($d(4) ^ $d(13)) & 1)]

	foreach nm [array names d] {
	    #puts -nonewline "$d($nm) "
	}
	#puts ""

	set j 0
	while { $j < 14 } {
	    
	    if { $d($j) > 9 } {
		set d($j) [expr $d($j)-10]
		incr d([expr $j+1]) 1
	    }

	    incr j 1
	}
	
	incr i 1
    }

    foreach nm [array names d] {
	#puts -nonewline "$d($nm) "
    }
    #puts ""

    set ::N ""
    foreach nm [array names d] {
	
	set ::N "$::N$d($nm)"
    }
puts "N='$::N'"
    
    
}

set i 0
while { $i < 1000 } {
    rnd $::N
    incr i 1
}
