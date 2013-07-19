#!/usr/bin/env python

# load ARB and os libraries
import ARB,os,sys

OK = "OK"
FAIL = "FAIL"

##
# define testcases
##

def open_nonexistant():
  """try opening nonexistant db"""
  err = FAIL
  try:
    arbdb = ARB.open("doesnotexist.arb","r")
  except Exception as e:
    err = OK
  print "Exception on attempt to open noexistant DB: " + err
  return err == OK

def count_spec_in_demo_arb():
  err = OK
  # open the demo database
  try:
    arbdb = ARB.open(os.environ["ARBHOME"] + "/demo.arb","r")
  except Exception as e:
    err = FAIL
  print "Opening demo.arb: " + err
  if (err != OK): return False

  # iterate through species and count
  num = 0
  err = OK
  for species in arbdb.all_species():
    num+=1
  if (num != 100):
    err = FAIL
  print "Counting species (expecting 100): " + err
  return err == OK

  arbdb.close()

##
# run tests
##

print "Testing ARB-Python interface:"
err = False
err = err or not open_nonexistant()
err = err or not count_spec_in_demo_arb()

if (err == True):
  print "Some tests failed!"
  sys.exit(1)
else:
  print "All tests completed successfully. :)"
    
