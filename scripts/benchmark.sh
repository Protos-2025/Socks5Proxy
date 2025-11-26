#!/bin/bash

rm -rf bench/results
mkdir -p bench/results/jmeter-html-report
jmeter -n -t bench/jmeter/concurrency.jmx -l bench/results/concurrency.jtl -j bench/results/jmeter.log -e -o bench/results/jmeter-html-report