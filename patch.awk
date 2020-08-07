# Translate our simplified diff
# into ed commands.

 # allow comment lines (ignored)
/^#/ { }

# open next file (saving previous)
/^\// { if (file) print "w";
         print "e " root $0;
         file=1;
       }

function search(text) {
  gsub(/\*/, "\\*", text);
  gsub(/\[/, "\\[", text);
  gsub(/\]/, "\\]", text);
  gsub(/\//, "\\/", text);
  return text;
}

# all patterns below trim the line this way
{ line=substr($0,3) }

# find this function header
/^@ /  { print "/" search(line) }

# find and remove this line
/^- /  { print "/" search(line) "/c"; inserting=1; }

# add this line
/^+ /  { if (inserting) print line "\n.";
         else prior=line
       }

# find this anchor line
/^  /  { if (prior) {
           print "/" search(line) "/i"
           print prior "\n.";
           prior=0;
         } else {
           print "/" search(line) "/a"
           inserting=1;
         }
       }

# empty lines separate diff blocks
/^$/   { if (prior) print "a\n" prior "\n.";
         prior=0; inserting=0; }

# write last file and exit
END    { print "wq" }

