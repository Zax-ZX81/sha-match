# sha-match
A simple system for cataloguing and managing files using SHA256.

SYNOPSIS

smatch [dimV] searchlist database

sfind [fiopsuvVx] dataset

scheck [V] search-file [-d database-file]

sconvert SHA256file

Description
sha-match  uses  SHA256  checksums to identify files, cataloguing collections of files (datasets) for later searches.  Dataset files can be compared to show files in common, or files unique to one set.

smatch compares two dataset files - a searchlist and a database, printing the filepaths of matching files from the searchlist to the screen.

sfind generates a dataset for the current directory.

scheck searches for a single file in a database.

sconvert converts a plain SHA256 file to a s2db file.

Development
I do most of my file management in the shell on GNU/Linux.  I don't mind tedious jobs like consolidating multiple backups - I've done such weeding jobs for myself, for my employer and as odd jobs for others.  In Linux that's meant using GDMap, FSLint and Duff; in Windows, WinDirStat and SearchMyFiles.
One abiding aggravation of such work is dealing with the same sets of files over and over.  My natural tendency, and clearly that of others, is to just make a copy of all the files I want to keep and putting it on a CD, DVD or external drive and then forgetting it.  This strategy is fine, but you can end up with three, four (or seven) copies of the same directory tree in one backup.  Then days after you've dealt with a redundancy nest like that you find the same thing on another external dive and have to make the same decisions about the same files all over again.  The files you keep for your archive are fine - you can use duplicate file finding software to get rid of the stuff from a nest that you already know you have - it's the junk, the files that might be valuable but have to be examined to make sure that are the biggest pain.  Every time you find some junk files and delete them you now no longer have any record of the files you've eliminated from consideration.  So when you find the that same nest for the third or fourth (or seventh) time you have to decide yet again whether those files are worth keeping.  MUST BE A BETTER WAY!
I can't remember exactly but I, at some point, generated a 200,000 file catalogue of work media files which included SHA256 checksums.  Perhaps so that I could check to see if a particular photo or video was identical to an archived one.  So, you do a checksum on a file, then grep for that checksum in the archive catalogue and if the file is in there grep will find it.  Great.  But what if you automated that?  So I wrote some (simple, because that's all I can manage) bash scripts that would generate and then compare lists with each other to show matching/redundant files.  That ws the beginning of lock-down.  I wasn't in any hurry.  So, it takes ten minutes to search for 10,000 files in the 200,000 file catalogue?  Who cares?  Once things were a bit more back to normal the speed started to annoy me while I was continuing with my one-archive-to-hold-them-all project for my own files.
I could see the basic shape of how to approach a dedicated program version of the bash scripts, so I started with FreeBASIC to see if I was kidding myself.  And to see how much faster it would be.  A few days of brutal concentration and the answers were: not really, and one minute.  Vindicated and possibly deluded by my sucess I went back to my old favourite enemy, C.  Having worked out most of the problems with BASIC, the C development was pretty smooth.  Also, malloc is much more satisfying to use that just dimming a huge array and hoping for the best.  One week and --- four seconds --- for smatch (C version) to do what sha-match (bash version) did in ten minutes.  Without hardly crashing at all.

Purpose
You find a set of files that are either grouped together as an archive/back-up, or that are just the bunch of crap you're trying to weed and make sense of.  You go into the base directory of that dataset and run sfind <dataset name>.  You then keep that file (say junk.s2db) so that at any point in the future you can compare files to its contents.  You then, in the case of a digital junk pile, weed until you've eliminated every file you don't need and stored - systematically - every file you want to keep.  The junk.s2db now essentially contains all the decisions you made during the weeding process.  If you come accross another vile nest of garbage, you can run sfind on that and then compare garbage.s2db to junk.s2db and smatch will spit out a list of all the files that you can delete without a moment's hesitation, confident in the knowledge that you been through all tht already, whether you deleted the files in junk or not.  The downfall of redundant file finders is that they don't record the decisions you made about the files you deleted.  Sha-match provides a way around that.

Files
.sfind_temp is a temporary file for storing file search.  Is deleted after use, but may linger in case of crash.
sf_filter is a file listing the names of files/directories in the current directory to be included/excluded from the file search.  Can be generated with 'ls  > sf_filter' and then edited as needed.
.scheck_db stores the name of the last database file that scheck used sucessfully.  This database will be used until another is specified.
dataset.sha256 - the two column (seperated by two spaces) output from sha256sum.
dataset.s2db - three column output, tab seperated.  1: SHA256, 2: filepath, 3: dataset.

Restrictions
The search in smatch requires that the searchlist is sorted by SHA256.  If it isn’t already, smatch will sort it.  The sort is slow.  As you’re likely to generate a dataset once but use it with smatch multiple times, sfind sorts its output by SHA256 checksums default.  That sort is also slow.  The search itself, however, is fast.

Portability
Wherever possible I used only standard C functions for all the programs.  The exception is sha256sum.  Both sfind and scheck use sha256sum via popen() to calculate SHA256 checksums.
All the programs build in GNU/Linux and Windows (with MinGW), and would probably on Mac too but that's untested.  Windows requires sha256sum in the command path to run sfind and scheck.  Th Windows native utilities certutil and get-filehash both produce SHA256 checksums, but certutil refuses to do checksums on empty files and I couldn't get get-filehash to run via popen().
sfind uses dirent.h to mimic the GNU/Linux find command, building a list of files/directories in the current directory and then feeding back on itself as it works down the list it has generated by itself.  The directories and other non-standard files are then filtered out with the regular files serving as the list for the checksum phase of the program

Bugs
I'm not a natural coder.  My debugging strategy mainly consists of putting in heaps of printf statements everywhere to try to figure out what the hell is going wrong.  Crashes in these programs now seem to be related to trying to close a file that isn't open or due to my misunderstanding the correct use of malloc.  sfind sends filenames to sha256sum enclosed in double quotes to account for spaces and other non-standard characters, but that sometimes fails - with '`' (backtick) for example.

Copyleft
If you want to steal this, I'll meet you in the carpark.

Author
Bob Hayward (Zax) zx81@zx81.nz
