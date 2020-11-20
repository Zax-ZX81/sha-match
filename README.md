## Sha-match
A simple system for cataloguing and managing files using SHA256.

### Synopsis

__smatch__ [dimV] _searchlist database_

__sfind__ [fiopsuvVx] _dataset_

__scheck__ [V] _search-file_ [-d database-file]

__sconvert__ _SHA256file_

### Description
__sha-match__ uses _SHA256_ checksums to identify files, cataloguing collections of files (___datasets___) for later searches.  ___Dataset___ files can be compared to show files in common, or files unique to one set.

__smatch__ compares two ___dataset___ files - a _searchlist_ and a _database_, printing the filepaths of matching files from the _searchlist_ to the screen.

__sfind__ generates a ___dataset___ for the current directory.

__scheck__ searches for a single file in a database.

__sconvert__ converts a plain _SHA256_ file to a _s2db_ file.

### Development
I do most of my file management in the shell on _GNU/Linux_.  I don't mind tedious jobs like consolidating multiple backups - I've done such weeding jobs for myself, for my employer and as odd jobs for others.  In Linux that's meant using _GDMap_, _FSLint_ and _Duff_; in _Windows_, _WinDirStat_ and _SearchMyFiles_.
One abiding aggravation of such work is dealing with the same sets of files over and over.  My natural tendency, and clearly that of others, is to just make a copy of all the files I want to keep and putting it on a CD, DVD or external drive and then forgetting it.  This strategy is fine, but you can end up with three, four (or seven) copies of the same directory tree in one backup.  Then days after you've dealt with a redundancy nest like that you find the same thing on another external dive and have to make the same decisions about the same files all over again.  The files you keep for your archive are fine - you can use duplicate file finding software to get rid of the stuff from a nest that you already know you have - it's the junk, the files that might be valuable but have to be examined to make sure that are the biggest pain.  Every time you find some junk files and delete them you now no longer have any record of the files you've eliminated from consideration.  So when you find the that same nest for the third or fourth (or seventh) time you have to decide yet again whether those files are worth keeping.  __Must be a better way!__
I can't remember exactly but I, at some point, generated a 200,000 file catalogue of work media files which included _SHA256_ checksums.  Perhaps so that I could check to see if a particular photo or video was identical to an archived one?  So, you do a checksum on a file, then _grep_ for that checksum in the archive catalogue and if the file is in there _grep_ will find it.  Great.  But what if you automated that?  So I wrote some (simple, because that's all I can manage) _bash_ scripts that would generate and then compare lists with each other to show matching/redundant files.  That was the beginning of lock-down.  I wasn't in any hurry.  So, it takes ten minutes to search for 10,000 files in the 200,000 file catalogue?  Who cares?  Once things were a bit more back to normal the speed started to annoy me while I was continuing with my one-archive-to-hold-them-all project for my own files.
I could see the basic shape of how to approach a dedicated program version of the _bash_ scripts, so I started with _FreeBASIC_ to see if I was kidding myself.  And to see how much faster it would be.  A few days of brutal concentration and the answers were: not really, and one minute.  Vindicated and possibly deluded by my sucess I went back to my old favourite enemy, _C_.  Having worked out most of the problems with _BASIC_, the _C_ development was pretty smooth.  Also, _malloc_ is much more satisfying to use that just dimming a huge array and hoping for the best.  One week and --- ___four seconds___ --- for __smatch__ (_C_ version) to do what __sha-match__ (_bash_ version) did in ten minutes.  Without hardly crashing at all.

### Purpose
You find a set of files that are either grouped together as an archive/back-up, or that are just the bunch of crap you're trying to weed and make sense of.  You go into the base directory of that dataset and run sfind <dataset name>.  You then keep that file (say junk.s2db) so that at any point in the future you can compare files to its contents.  You then, in the case of a digital junk pile, weed until you've eliminated every file you don't need and stored - systematically - every file you want to keep.  The junk.s2db now essentially contains all the decisions you made during the weeding process.  If you come accross another vile nest of garbage, you can run sfind on that and then compare garbage.s2db to junk.s2db and smatch will spit out a list of all the files that you can delete without a moment's hesitation, confident in the knowledge that you been through all that already, whether you deleted the files in junk or not.  The downfall of redundant file finders is that they don't record the decisions you made about the files you deleted.  Sha-match provides a way around that.  To delete files you just type smatch garbage.s2db junk.s2db | tr '\012' '\000' | xargs -0 rm -v and the output from smatch will be fed into the delete command.  You can optionally send the smatch output through sort to a file (match garbage.s2db junk.s2db | sort > maybe-delete-these.txt), edit the file to remove files you have second thoughts about, and then pipe that file to delete (cat maybe-delete-these.txt | tr '\012' '\000' | xargs -0 rm -v).
  
### In depth
Before treating any searchlist or database file as legitimate, all the programs do a simple check of the first line of the file.  It runs through the first 64 characters, checking that they're hexidecimals and then decides on the filetype based on character 65.  If it's a space, then sha256; if it's a tab, then s2db.  Not foolproof, but effective.

#### smatch
smatch compares two dataset files - a searchlist and a database, printing the filepaths of matching files from the searchlist to stdout.  If you run it in the directory you ran the sfind corresponding to the searchlist in, all the files on stdout with have the right pathnames to send via a pipe to something else, like rm (1).  If you want to see the filenames of the matches in the database file, use -d.  By default smatch shows only one match for each file in the searchlist; use -m to change this.  -d and -m can be used together.  To generate a list of all the files in the database that aren't in the searchlist use -i.  This inverse output prints the two (in the case of \*.sha256) or three (in the case of \*.s2db) columns from the database which can be piped to a file to create a new s2db file.  You can join s2db files together to create a master database file so that one search can be done instead of many.  This can be done using cat 2020_Nov_\*.s2db > 2020_Nov_EVERYTHING.s2db.
The dataset column in s2db files has two purposes: one, to keep track of where a file came from; two, to guard against unintentional deletion.  If you have a series of databases of all your digital photos that you've joined together (AllMyPhotos.s2db) and you compare a subset of that (PhonePhotos.s2db), you will have a one hundred percent hit rate.  If you find yourself in the PhonePhotos directory and accidently pipe those results to delete them, then you'll lose everything.  smatch checks each match it finds to make sure that the datasets for the pairs of files don't match.  In the above example smatch will print files that match from different datasets in the database but at the end will give the message "Some results excluded because of dataset conflict".
smatch loads the searchlist into a struct array, sorted by SHA256.  It builds an index of the array, marking the boundaries between the first hex characters of the checksums - sixteen in all.  It then moves through the database one line at a time, finding the correct bracket of checksums in the searchlist array by using the first character of the database line checksum.  So each line of the database is only searched for in a sixteenth of the searchlist array.

#### sfind
sfind produces the datasets used by smatch and scheck.  Its output is roughly the same as typing find -type f | xargs sha256sum > dataset.sha256 on the Linux command line.
sfind generates a list of files/directories in the current directory that then feeds back on itself as it works down its own list.  The directories and other non-standard files are then removed with the regular files serving as the list for the checksum phase of the program.  When using -i or -x, the first stage of building the file list will check for all the items in ./sf_filter and include or exclude them accordingly.  The filter doesn't work for items in subdirectories.

#### scheck
scheck does the same as smatch only with a single file.  It records the last database it used successfully and will use that if none is specified.  It will print multiple results if it finds them, showing the dataset for each.

#### sconvert
sconvert converts a plain SHA256 file to a s2db file.  It uses the filename to fill the dataset field.

### Files
.sfind_temp is a temporary file for storing file search.  Is deleted after use, but may linger in case of crash.
sf_filter is a file listing the names of files/directories in the current directory to be included/excluded from the file search.  Can be generated with 'ls  > sf_filter' and then edited as needed.
.scheck_db stores the name of the last database file that scheck used sucessfully.  This database will be used until another is specified.
dataset.sha256 - the two column (seperated by two spaces) output from sha256sum.
dataset.s2db - three column output, tab seperated.  1: SHA256, 2: filepath, 3: dataset.

### Restrictions
The search in smatch requires that the searchlist is sorted by SHA256.  If it isn’t already, smatch will sort it.  The sort is slow.  As you’re likely to generate a dataset once but use it with smatch multiple times, sfind sorts its output by SHA256 checksums default.  That sort is also slow.  The search itself, however, is fast.

### Portability
I used only standard C functions for all the programs.  Both sfind and scheck use sha256sum via popen() to calculate SHA256 checksums.
All the programs build in GNU/Linux and Windows (with MinGW), and would probably on Mac too, but that's untested.  Windows requires sha256sum in the command path to run sfind and scheck.  The Windows native utilities certutil and get-filehash both produce SHA256 checksums, but certutil refuses to do checksums on empty files and I couldn't coax get-filehash to run via popen().
sfind uses dirent.h to mimic the GNU/Linux find command, building a list of files/directories in the current directory to generates checksums on.

### Bugs
I'm not a natural coder.  My debugging strategy mainly consists of putting in heaps of printf statements everywhere to try to figure out what the hell is going wrong.  Crashes in these programs now seem to be related to trying to close a file that isn't open or due to my misunderstanding the correct use of malloc.  sfind sends filenames to sha256sum enclosed in double quotes to account for spaces and other non-standard characters, but that sometimes fails - with '\`' (backtick) for example.

### Copyleft
If you want to steal this, I'll meet you in the carpark.

### Author
Bob Hayward (Zax) zx81@zx81.nz
