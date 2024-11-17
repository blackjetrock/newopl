#!/usr/bin/tclsh
#
# Run tests to check that all is OK compared to the Psion translation
#
#
#
################################################################################

set ::PASS 0
set ::FAIL 0

proc compare_results {basename f1 f2} {

    set f1fp [open $f1]
    set f1txt [read $f1fp]
    close $f1fp

    set f2fp [open $f2]
    set f2txt [read $f2fp]
    close $f2fp

    set identical 1

    foreach l1 [split $f1txt "\n"] l2 [split $f2txt "\n"] {
	if { [string first "Filename" $l1] != -1 } {
	    continue
	}

	if { [string first "File ext" $l1] != -1 } {
	    continue
	}
	
	if { [string compare $l1 $l2] != 0 } {
	    set identical 0
	}
    }

    if { $identical } {
	set r " identical"
	incr ::PASS 1
    } else {
	set r " not identical"
	incr ::FAIL 1
    }
    
    puts [format "%-20s   %s" $basename\.opl $r]
}


# Find all files that are nopl translation tests

set TR_TEST_FILES [glob *_psion.tr.test]

# Rebuild the nopl version of the test file
puts "\n"

foreach file $TR_TEST_FILES {

    if { [regexp -- {(.*)_psion.tr.test} $file all basename] } {
	# Re-translate the source to get the file we need to test against
	exec ./newopl_tran $basename\.opl
	exec ./newopl_objdump ob3_nopl.bin > $basename\_nopl.tr.test

	# Compare the results
	compare_results $basename $file $basename\_nopl.tr.test
    }
}

puts "\n$::PASS tests passed $::FAIL tests failed"
