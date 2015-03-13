#!/usr/bin/awk -f
BEGIN {
  in_trees = in_translate = 0;
}
/begin trees;/ { 
  in_trees = 1; 
}
in_translate && /;/ {
  in_translate = 0
}
in_translate {
  sub(/,$/,"")
  map[$1]=$2
}
in_trees && /translate/ { 
  in_translate = 1;
}
in_trees && /\[/ {
  in_comment = 1;
}
in_comment
in_comment && /\]$/ {
  in_comment = 0;
}
in_trees && /tree [^ ]* =/ {
  for (i in map) {
     a=0;
     a+=gsub("[(]"i":","("map[i]":", $4)
     a+=sub(","i":",","map[i]":", $4)
     if (a!=1) print i " " map[i]
  }
  print $4
 
}
