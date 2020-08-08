# Translate our simplified diff into ed commands.
# (`root` is an input variable, the location of the files to patch)

function search(text) {
  # create a literal search string for ed (escaping to prevent regex)
  gsub(/\*/, "\\*", text);
  gsub(/\[/, "\\[", text);
  gsub(/\]/, "\\]", text);
  gsub(/\//, "\\/", text);
  return text;
}

# pattern   condition        output (ed commands)                        state                description
# -------   ---------------  ------------------------------------------  ------------------   ---------------------------------
/^#/      {                                                                                 } # ignore comments
          {                                                              line=substr($0,3)  } # trim meta characters from line
/^\//     { if (file)        print "w";                                                       # save previous file
                             print "e " root $0;                         file=1;            } # edit file
/^@ /     {                  print "/" search(line)                                         } # find this function header
/^- /     {                  print "/" search(line) "/c";                inserting=1;       } # find and remove this line
/^+ /     { if (inserting)   print line "\n.";                                                # add this line
            else                                                         prior=line;        } # remember this line
/^  /     { if (prior)     { print "/" search(line) "/i\n" prior "\n.";  prior=0; }           # insert before this anchor line
            else           { print "/" search(line) "/a";                inserting=1; }     } # insert after this anchor line
/^$/      { if (prior)       print "a\n" prior "\n.";                    prior=inserting=0; } # empty lines separate diff blocks
END       {                  print "wq"                                                     } # write last file and exit

