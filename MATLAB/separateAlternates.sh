#!/bin/bash

sepAltExec=/Users/max/Library/Developer/Xcode/DerivedData/connection_compare-azguassdtbouupaqtonbeyldvspy/Build/Products/Debug/seperateAlternates

for I in "Working Files/Alternates Blobs/"*.png; do
	$sepAltExec "$I" "Extracted Characters/Alternates"
done	