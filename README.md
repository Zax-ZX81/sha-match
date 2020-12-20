## Sha-match
A simple system for cataloguing and managing files using _SHA256_.

### Synopsis

__smatch__ [dimV] _searchlist database_

__sfind__ [fiopsuvVx] _dataset_

__scheck__ [V] _search-file_ [-d _database-file_]

__sconvert__ _sha256file_

### Description
__sha-match__ uses _SHA256_ checksums to identify files, cataloguing collections of files (___datasets___) for later searches.  ___Dataset___ files can be compared to show files in common, or files unique to one set.  __smatch__ only compares text files - it doesn't need access to the files the ___datasets___ relate to.

__smatch__ compares two ___dataset___ files - a _searchlist_ and a _database_, printing the filepaths of matching files from the _searchlist_ to the screen.

__sfind__ generates a ___dataset___ for the current directory.

__scheck__ searches for a single file in a _database_.

__sconvert__ converts a plain _sha256_ file to a _s2db_ file.

### Development
I do most of my file management in the shell in _GNU/Linux_.  I don't mind tedious jobs like consolidating multiple backups - I've done such weeding jobs for myself, for my employer and as odd jobs for others.  In _Linux_ that's meant using _GDMap_, _FSLint_ and _Duff_; in _Windows_, _WinDirStat_ and _SearchMyFiles_.
One abiding aggravation of such work is dealing with the same sets of files over and over.

My natural tendency, and clearly that of others, is to just make a copy of all the files I want to keep, put it on a CD, DVD or external drive and then forget about it.  This strategy is fine, but you can end up with three or four (or seven) copies of the same directory tree in one place.  Then days after you've dealt with a redundancy nest like that you find the same thing on another external drive and have to make the same decisions about the same files all over again.

The files you keep for your archive are fine - you can use duplicate file finding software to get rid of the stuff from a nest that you already know you have - it's the junk, the files that might be valuable but have to be examined to make sure, that are the biggest pain.  Every time you find some junk files and delete them you now no longer have any record of the files you've eliminated from consideration.  So when you find the that same nest for the third or fourth (or seventh) time you have to decide yet again whether those files are worth keeping.  __Must be a better way!__

I can't remember exactly but I, at some point, generated a 200,000 file catalogue of work media files which included _SHA256_ checksums.  Perhaps so that I could check to see if a particular photo or video was identical to an archived one without having to access that 800GB of files?  So, you do a checksum on a file, then `grep` for that checksum in the archive catalogue and if the file is in there `grep` will find it.  Great.  But what if you automated that? 

So I wrote some (simple, because that's all I can manage) _bash_ scripts that would generate and then compare lists with each other to show matching/redundant files.  That was the beginning of lock-down.  I wasn't in any hurry.  So, it takes ten minutes to search for 10,000 files in the 200,000 file catalogue?  Who cares?  Once things were a bit more back to normal the speed started to annoy me while I was continuing with my one-archive-to-hold-them-all project for my own files.

I could see the basic shape of how to approach a dedicated program version of the _bash_ scripts, so I started with _FreeBASIC_ to see if I was kidding myself.  And to see how much faster it would be.  A few days of brutal concentration and the answers were: not really, and ten times.  Vindicated and possibly deluded by my success I went back to my old favourite enemy, _C_.  Having worked out most of the problems with _BASIC_, the _C_ development was pretty smooth.  Also, _malloc_ is much more satisfying to use than just dimming a huge array and hoping for the best.  One week and --- ___four seconds___ --- for __smatch__ (_C_ version) to do what __sha-match__ (_bash_ version) did in ten minutes.  Without hardly crashing at all.

From there it was all feature-creep, writing support programs and swearing at bugs.  Also learning to use `make`, _GitHub_ and writing `man` pages and read-mes.

### Purpose
You find a set of files that are either grouped together as an archive/back-up, or that are just the bunch of crap you're trying to weed and make sense of.  You go into the base directory of that ___dataset___ and run __sfind__ _dataset-name_.  You then keep that file (say _junk.s2db_) so that at any point in the future you can compare files to its contents.  You then, in the case of a digital junk pile, weed until you've eliminated every file you don't need, and stored - systematically, of course - every file you want to keep.  The _junk.s2db_ now essentially contains all the decisions you made during the weeding process.

If you come accross another vile nest of garbage, you can run __sfind__ on that and then compare _garbage.s2db_ to _junk.s2db_ and __smatch__ will spit out a list of all the files that you can delete without a moment's hesitation, confident in the knowledge that you been through all that already, whether you deleted the files in junk or not.  The downfall of redundant file finders is that they don't record the decisions you made about the files you deleted.  __Sha-match__ provides a way around that.

To delete files you just type `smatch garbage.s2db junk.s2db | tr '\012' '\000' | xargs -0 rm -v` and the output from __smatch__ will be fed into the delete command.  You can optionally send the __smatch__ output through `sort` to a file (`smatch garbage.s2db junk.s2db | sort > maybe-delete-these.txt`), edit to remove files you have second thoughts about, and then pipe that file to delete (`cat maybe-delete-these.txt | tr '\012' '\000' | xargs -0 rm -v`).
  
### In depth
Before treating any _searchlist_ or _database_ file as legitimate, all the programs do a simple check of the first line of the file.  They run through the first 64 characters, checking that they're hexadecimals and then decide on the filetype based on character 65.  If it's a `space`, then _sha256_; if it's a `tab`, then _s2db_.  Not foolproof, but effective.  See the `man` page for more details on switches, particularly for __sfind__.

#### smatch
__smatch__ compares two ___dataset___ files - a _searchlist_ and a _database_, printing the filepaths of matching files from the _searchlist_ to _stdout_.  If you run it in the directory you ran the __sfind__ corresponding to the _searchlist_ in, all the files on _stdout_ will have the right pathnames to send via a pipe to something else, like `rm (1)`.  If you want to see the filenames of the matches in the _database file_, use __-d__.  By default __smatch__ shows only one match for each file in the _searchlist_; use __-m__ to change this.  __-d__ and __-m__ can be used together.  To generate a list of all the files in the _database_ that aren't in the _searchlist_ use __-i__.  This inverse output prints the two (in the case of _sha256_) or three (in the case of _s2db_) columns from the _database_ which can be piped to a file to create a new _s2db_ file.  You can join _s2db_ files together to create a master _database_ file so that one search can be done instead of many.  This can be done using `cat 2020_Nov_*.s2db > 2020_Nov_EVERYTHING.s2db`.

The ___dataset___ column in _s2db_ files has two purposes: one, to keep track of where a file came from; two, to guard against unintentional deletion.  If you have a series of databases of all your digital photos that you've joined together (`AllMyPhotos.s2db`) and you compare a subset of that (`PhonePhotos.s2db`), you will have a one hundred percent hit rate.  If you find yourself in the `PhonePhotos` directory and accidently pipe those results to delete them, then you'll lose everything.  __smatch__ checks each match it finds to make sure that the ___datasets___ for the pairs of files don't match.  In the above example __smatch__ will print files that match from different ___datasets___ in the _database_ but at the end will give the message "Some results excluded because of dataset conflict".

__smatch__ loads the _searchlist_ into a struct array, sorted by _SHA256_.  It builds an index of the array, marking the boundaries between the first hex characters of the checksums - sixteen in all.  It then moves through the _database_ one line at a time, finding the correct bracket of checksums in the _searchlist_ array by using the first character of the _database_ line checksum.  So each line of the _database_ is only searched for in a sixteenth of the _searchlist_ array.

#### sfind
__sfind__ produces the ___datasets___ used by __smatch__ and __scheck__.  Its output is roughly the same as typing `find -type f | xargs sha256sum > dataset.sha256` on the _Linux_ command line.

__sfind__ generates a list of files/directories in the current directory that then feeds back on itself as it works down its own list.  The directories and other non-standard files are then removed with the regular files serving as the list for the checksum phase of the program.  When using __-i__ or __-x__, during the building the file list it will check for all the items in `./sf_filter` and include or exclude them accordingly.

#### scheck
__scheck__ does the same as __smatch__ only with a single file.  It records the last _database_ it used successfully and will keep using that until another _database_ that passes the verification check is specified.  It will print multiple results if it finds them, showing the ___dataset___ for each.

#### sconvert
__sconvert__ converts a plain _sha256_ file to a _s2db_ file.  It uses the filename to fill the ___dataset___ field.

### Files
`sf_filter` is a file listing the names of files/directories in the current directory to be included/excluded from the file search.  Can be generated with `ls  > sf_filter` and then edited as needed.

`.scheck_db` stores the name of the last _database_ file that __scheck__ used sucessfully.  This _database_ will be used until another is specified.

_dataset.sha256_ - the two column (separated by two spaces) output from _sha256sum_. 

_dataset.s2db_ - three column output, `tab` separated.  1: SHA256, 2: filepath, 3: dataset.

### Restrictions
The search in __smatch__ requires that the _searchlist_ is sorted by _SHA256_.  If it isn’t already, __smatch__ will sort it.  The sort is slow.  As you’re likely to generate a ___dataset___ once but use it with __smatch__ multiple times, __sfind__ sorts its output by _SHA256_ checksums by default.  That sort is also slow.  The search itself, however, is fast.

### Portability
I used only standard _C_ functions for all the programs.  Both __sfind__ and __scheck__ use popen() to get SHA256SUMs - from `sha256sum` in _Linux_ and `certutil` in _Windows_.
All the programs build in _GNU/Linux_ and _Windows_ (with _MinGW_), and would probably on _Mac_ too, but that's untested.  The output in _Linux_ is coloured; in _Windows_ it's black and white.

__sfind__ uses _dirent.h_ to mimic the _GNU/Linux_ `find` command, building a list of files/directories in the current directory to generate checksums for.

### Bugs
I'm not a natural coder.  My debugging strategy mainly consists of putting in heaps of _printf_ statements everywhere to try to figure out what the hell is going wrong.  Crashes in these programs now seem to be related to trying to close a file that isn't open or due to my misunderstanding the correct use of _malloc_.  __sfind__ sends filenames to `sha256sum` enclosed in double quotes to account for spaces and other non-standard characters, but that sometimes fails - with \` (backtick) for example.

### Copyleft
If you want to steal this, I'll meet you in the carpark.

### Author
Bob Hayward (Zax) zx81@zx81.nz
