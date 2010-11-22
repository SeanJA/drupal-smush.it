Dependencies
==

* Drupal 6.x
* Upload module
* json functions
* curl extension
* PHP 5

Install
==

* Add a folder to your modules directory called `smush_it`
* Put these files in it.
* Activate the module.

Features
==

* Will run on node create / update when new images are found
* Can run on cron
  * You can set the number of images to be processed
  * This can also be disabled
* Can run in testing mode for local development
  * When on, you point it at a web accessible file
  * This file will replace your files (the files will still be around in the `.old` form)

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

* If the file is a gif, it could come back as a png
  * If it does, the file will still be `/path/to/file.gif` but the mimetype in the file system will be changed to `image/png`
    * This means that files you have linked to on your site will not break

TODO
==
* Make smush on create / update optional (it could timeout for a large number of images)
* Option to smush individual images one by one