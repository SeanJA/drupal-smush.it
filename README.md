Dependencies
==

* Drupal 6.x
* Upload module
* json functions
* curl extension
* PHP 5 (for exceptions and __construct niceties)
* bc_math functions for the stats page (everything is stored as bytes which could be a really big number when summed)

Install
==

* Add a folder to your modules directory called `smush_it`
* Put these files in it.
* Activate the module.

Features
==

* Will run on node create / update when new images are found
* Can run on cron (default: off)
  * You can set the number of images to be processed
* Can run on insert/update of a node (default: on)
  * If your insert/updates are timing out (or you plan on adding a pile of images to nodes) you will want to disable this
    * _Note_: if you disable this, you should enable the cron job

* Can run in testing mode for local development
  * When on, you point it at a web accessible file
  * This file will replace your files (the files will still be around in the `.old` form)

* Can smush individual images from the `Files` tab

* Added stats page
  * Contains:
    * Original Image Size:
      * Original image size before being smushed
    * Bytes Saved:
      * Total bytes saved by smush.it (goes up to YB)
    * Smushed Size:
      * Total image size after being smushed (goes up to YB)
    * Images Smushed:
      * Total images processed by smush.it
    * Average Bytes Saved:
      * Average number of bytes saved per image (goes up to YB)
    * Images:
      * Images that have been uploaded
    * Images To Process:
      * Images left to be processed

Usage
==

* This module will copy your original image to a .old file.
* Then it asks smush.it if there is a smaller version of the file available
* Then it pulls down the smushed file if there is one
* It will then record the difference in size in the `smush_it` table
* The new image replaces the old one
  * the old one is still saved as `x.old` incase of a problem

Notes
==

* If the file is a gif, it could come back as a png (browsers generally don't care what kind of image it is)
  * If it does, the file will still be `/path/to/file.gif` but the mimetype in the file system will be changed to `image/png`
    * This means that files you have linked to on your site will not break

TODO
==
* .old file cleanup
* replace with .old file