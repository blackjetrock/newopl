#!/usr/bin/tclsh
#
# Run tests to check that all is OK compared to the Psion translation
#
#
#
################################################################################

set ::PASS 0
set ::FAIL 0
set ::OPL_LINECOUNT 0

proc compare_results {basename f1 f2} {

    set f1fp [open $f1]
    set f1txt [read $f1fp]
    close $f1fp

    set f2fp [open $f2]
    set f2txt [read $f2fp]
    close $f2fp

    set identical 1

    set lineno 0
    
    foreach l1 [split $f1txt "\n"] l2 [split $f2txt "\n"] {
	if { [string first "Filename" $l1] != -1 } {
	    set line_identical($lineno) 1
	    continue
	}

	if { [string first "File ext" $l1] != -1 } {
	    set line_identical($lineno) 1
	    continue
	}
	
	if { [string compare $l1 $l2] != 0 } {
	    set identical 0
	    set line_identical($lineno) 0
	} else {
	    
	    # Identical, mark as such
	    set line_identical($lineno) 1
	}
	
	incr lineno 1
    }


    
    if { $identical } {
	set r " identical"
	incr ::PASS 1
    } else {
	set r " not identical"
	incr ::FAIL 1
    }

    
    # Now find blocks of non-identical lines
    set last_state 1
    set num_diffblocks 0
    
    for {set i 0} {$i<$lineno} {incr i 1} {
	if { [set line_identical($i)] && !$last_state } {
	    set diffblock_end($num_diffblocks) $i
	    incr num_diffblocks 1
	}

	if { !$line_identical($i) && $last_state } {
	    set diffblock_start($num_diffblocks) $i
	}

	set last_state $line_identical($i)
    }

    if { $::OPOK } {
	set opok_str " "
    } else {
	set opok_str "*"
    }

    puts -nonewline [format "%-20s   %-14s %s" $basename\.opl $r $opok_str]
    
    puts -nonewline " "
    foreach db [array names diffblock_start] {
	puts -nonewline "[set diffblock_start($db)]-[set diffblock_end($db)]  "
    }
    puts ""
    

}


# Find all files that are nopl translation tests

set EX_TEST_FILES [glob extst*.opl]

foreach file [lsort $EX_TEST_FILES] {
    puts $file
}

# Rebuild the nopl version of the test file
puts "\n"

foreach file [lsort $EX_TEST_FILES] {
    if { [regexp -- {(.*).opl} $file all basename] } {
	puts "Testimg..."
	set op [exec ./newopl_tran $basename\.opl]
	puts $op
	set op [exec ./newopl_exec ob3_nopl.bin]
	puts $op
	
    }
}


puts "\n$::PASS tests passed $::FAIL tests failed"
puts "$::OPL_LINECOUNT lines of OPL tested"
