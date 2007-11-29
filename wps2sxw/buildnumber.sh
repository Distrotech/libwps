#!/bin/sh

# Script to create the BUILDNUMBER used by compile-resource. This script
# needs the script createBuildNumber.pl to be in the same directory.

{ ./createBuildNumber.pl \
	wps2sxw/wps2sxw-build.stamp 
#Success
exit 0
}
#unsucessful
exit 1