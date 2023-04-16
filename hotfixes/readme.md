# Hotfix data format
For this app, we don't really need a super easy to use file format. If anything fact, being more
opaque helps make it harder for someone to cheat and sneak in some modded hotfixes. Because of this,
we can optimize some stuff for our use case.

Firstly, we preprocess the hotfix files so that we can essentially just wstrcpy them. The first 4
bytes of each hotfix file are the number of hotfixes, as a uint32. After that, the file consists of
null-separated key-value pairs for each hotfix, in utf16.

```
02 00 00 00                             # This file contains two hotfixes
41 00 00 00 41 00 41 00 00 00           # The first is equivalent to "A": "AA"
42 00 42 00 00 00 43 00 43 00 00 00     # The second is equivalent to "BB": "CC"
```
Technically on linux/mac these would have to be 4-byte chars, but it's not like the rest of the
program will work on other platforms anyway.

Next, we want to get a name for each hotfix. We just use the raw filename for this. We sort files
alphabetically, so to allow for custom ordering, we designate a semicolon as a separator character.
The whole filename is used for sorting, but only the text after the first semicolon is displayed -
e.g. `000;Speedrun Mod` displays as `Speedrun Mod`, and should sort to the top of the list (assuming
everything's using a numeric prefix).

Finally, since it's a lot of files, which are a little annoying to move around, and since they
actually take a decent amount of space (180mb for bl3), we throw them in a `.tar.gz`. Since we want
to be able to link multiple sets of hotfixes right next to each other (for each game) though, rather
than using a hardcoded `dehotfixer.tar.gz`, we change the extension to `.hfdat`, and try load the
first file with that extension which we see.
