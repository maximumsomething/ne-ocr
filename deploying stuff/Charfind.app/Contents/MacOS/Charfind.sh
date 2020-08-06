#!/bin/bash
if JAVA_HOME="$(/usr/libexec/java_home -v 1.8)"; then
	$JAVA_HOME/bin/java -jar "$(dirname $0)/../Resources/line_compare.jar"
else
	osascript -e 'display alert "You must have Java 8 installed to run this application."'
fi

