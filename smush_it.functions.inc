<?php

/**
 * Fake namespacing of the functions (allows for private functions to hide them from the global scope)
 */
class smush_it {

  /**
   * Load an image from it's file_id (fid)
   * @param int $file_id
   * @return array
   */
  private static function load_file($file_id){
    $result = db_query('SELECT * FROM {files} WHERE fid = %d', $file_id);
    $file = db_fetch_array($result);
    return $file;
  }

  /**
   * Set an image to be ignored by smush_it's cron job
   * @param int $file_id
   * @return boolean
   */
  static function ignore_image($file_id){
    $file = self::load_file($file_id);
    $smush_it = array(
      'space_saved' => 0,
      'fid' => $file['fid'],
    );
    $smush_it_object = (object) $smush_it;
    drupal_write_record('smush_it', $smush_it_object);
    return true;
  }

  /**
   * Revert an image to it's unsmushed state
   * @param int $file_id
   * @return boolean
   */
  static function revert_image($file_id){
    $file = self::load_file($file_id);
    $root_path = self::document_root() . '/';
    $filepath = $root_path . $file['filepath'];
    //don't wipe out files that do not have backups
    if(file_exists($filepath.'.old')){
      unlink($filepath);
      rename($filepath . '.old', $filepath);
    }
    list($width, $height, $type, $attr) = getimagesize($filepath);
    $file['mimetype'] = image_type_to_mime_type($type);
    $file['filesize'] = filesize($filepath);
    
    $smush_it = array(
      'space_saved' => 0,
      $smush_it['fid'] => $file['fid'],
    );

    $smush_it_object = (object) $smush_it;
    $file_object = (object) $file;
    db_query('DELETE FROM {smush_it} WHERE fid = %d', $file['fid']);
    drupal_write_record('files', $file_object, 'fid');
    return true;
  }

  /**
   * Get the total space saved
   * @return string
   */
  static function _total_saved() {
    $result = db_query('SELECT SUM(space_saved) as total FROM {smush_it};');
    $result = db_fetch_array($result);
    return $result['total'];
  }

  /**
   * Count the total image size
   * @return string
   */
  static function _image_size() {
    $result = db_query("SELECT SUM(filesize) as total FROM {files} WHERE filemime IN ('image/jpeg', 'image/png', 'image/gif');");
    $result = db_fetch_array($result);
    return $result['total'];
  }

  /**
   * The number of smushed files
   * @return string
   */
  static function _smushed_count() {
    $result = db_query("SELECT COUNT(*) as total FROM {smush_it};");
    $result = db_fetch_array($result);
    return $result['total'];
  }

  /**
   * The number of images total
   * @return string
   */
  static function _image_count() {
    $result = db_query("SELECT COUNT(*) as total FROM {files} WHERE filemime IN ('image/jpeg', 'image/png', 'image/gif');");
    $result = db_fetch_array($result);
    return $result['total'];
  }

  /**
   * Get the document root for the current Drupal installation.
   * $_SERVER['DOCUMENT_ROOT'] is not reliable across all
   * systems, so we need a way to get the correct value.
   * @staticvar string $root The value that will be returned
   * @return string
   */
  static function document_root() {
    static $root;
    if (!$root) {
      //@BUG: there has to be a better way to do this part...
      $absolute_dir = dirname(__FILE__);
      $relative_dir = drupal_get_path('module', 'smush_it');
      $root = substr($absolute_dir, 0, -1 * (1 + strlen($relative_dir)));
    }
    return $root;
  }

  /**
   * Format a number of bytes nicely for output (B, KB, MB, GB, TB...)
   * @param int $bytes Number of bytes
   * @param string $unit units you want it back in (default is to choose the biggest one that fits)
   * @param int $decimals number of decimal places
   * @return string
   */
  static function byte_format($bytes, $unit = "", $decimals = 2) {
    return _smush_it_byte_format($bytes, $unit, $decimals);
  }

  /**
   * Push an image through the smush.it api
   * @global string $base_url
   * @staticvar SmushIt $s The smush.it object
   * @staticvar bool $smush_it_testing Whether or not we are in test mode
   * @staticvar string $smush_it_test_image The image that will be used in test mode
   * @param file $file The file being processed
   */
  static function smush_image($file_id) {
    $file = self::load_file($file_id);
    global $base_url;
    static $s, $smush_it_testing, $smush_it_test_image;
    if (!$s) {
      $s = new SmushIt();
      $smush_it_testing = variable_get('_smush_it_testing', 0);
      $smush_it_test_image = variable_get('_smush_it_test_image', null);
    }
    $filepath = $file['filepath'];

    if (!$smush_it_testing) {
      //use the actual file
      $file_url = $base_url . DIRECTORY_SEPARATOR . $filepath;
    } else {
      //use a web accessible file set in the settings section
      $file_url = $smush_it_test_image;
    }
    try {
      $q = db_query('SELECT smushid FROM {smush_it} WHERE fid = %d', $file['fid']);
      $exists = db_fetch_array($q);
      if (!empty($exists['smushid'])) {
        throw new Smush_exception('The file has already been smushed', null);
      }
      $smushed = $s->compress($file_url);
      $file = self::save_file($file, $smushed);
      //update the mimetype just incase something actually uses it
      list($width, $height, $type, $attr) = getimagesize($smushed->src);
      $file['filemime'] = image_type_to_mime_type($type);

      //drupal_write_record expects an object, not an array
      $file_object = (object) $file;
      drupal_write_record('files', $file_object, array('fid'));
      //store the smushed info in the database as well so the image isn't processed over and over again
      $smush_it_object = (object) array(
          'fid' => $file['fid'],
          'space_saved' => $smushed->src_size - $smushed->dest_size
      );
      drupal_write_record('smush_it', $smush_it_object);
    } catch (Smush_exception $e) {
      //no savings means that the file is as small as it can be (not really an error, but returned as one by the api)
      if ($e->getMessage() == 'No savings') {
        //store the smushed info in the database as well so the image isn't processed over and over again
        $smush_it_object = (object) array(
            'fid' => $file['fid'],
            'space_saved' => 0
        );
        drupal_write_record('smush_it', $smush_it_object);
      } else {
        //An actual error! Send it to the user
        drupal_set_message($e->getMessage() . ' => ' . $file['filename'], 'error');
        return false;
      }
    }
    return true;
  }

  /**
   * Save the smushed file
   * @param file $file
   * @param smushed $smushed
   * @return file $file
   */
  private static function save_file($file, &$smushed) {
    $root_path = smush_it::document_root() . '/';
    $remote = fopen($smushed->dest, "rb");
    $new_file = $root_path . $file['filepath'] . '.new';
    $original_file = $root_path . $file['filepath'];
    $old_file = $root_path . $file['filepath'] . '.old';
    //read the file in bit by bit
    $temp = fopen($new_file, 'w+');
    while (!feof($remote)) {
      fwrite($temp, fread($remote, 8192));
    }
    fclose($temp);
    fclose($remote);

    //backup the old file (.old) and replace it with the new file
    rename($original_file, $old_file);
    rename($new_file, $original_file);
    $file['filesize'] = $smushed->dest_size;
    return $file;
  }

}

if (BC_LOADED) {

  /**
   * Format a number of bytes nicely for output (B, KB, MB, GB, TB...)
   * @param int $bytes Number of bytes
   * @param string $unit units you want it back in (default is to choose the biggest one that fits)
   * @param int $decimals number of decimal places
   * @return string
   */
  function _smush_it_byte_format($bytes, $unit = "", $decimals = 2) {
    $units = array('B' => 0, 'KB' => 1, 'MB' => 2, 'GB' => 3, 'TB' => 4, 'PB' => 5, 'EB' => 6, 'ZB' => 7, 'YB' => 8);

    $value = 0;
    if ($bytes > 0) {
      // Generate automatic prefix by bytes
      // If wrong prefix given
      if (!array_key_exists($unit, $units)) {
        $pow = floor(log($bytes) / log(1024));
        $unit = array_search($pow, $units);
      }

      // Calculate byte value by prefix
      $value = $bytes / pow(1024, floor($units[$unit]));
    }

    // If decimals is not numeric or decimals is less than 0
    // then set default value
    if (!is_numeric($decimals) || $decimals < 0) {
      $decimals = 2;
    }

    // Format output
    return sprintf('%.' . $decimals . 'f ' . $unit, $value);
  }

} else {
  /**
   * Format a number of bytes nicely for output (B, KB, MB, GB, TB...)
   * @param int $bytes Number of bytes
   * @param string $unit units you want it back in (default is to choose the biggest one that fits)
   * @param int $decimals number of decimal places
   * @return string
   */
  function _smush_it_byte_format($bytes, $unit = "", $decimals = 2) {
    $units = array('B' => 0, 'KB' => 1, 'MB' => 2, 'GB' => 3, 'TB' => 4, 'PB' => 5, 'EB' => 6, 'ZB' => 7, 'YB' => 8);

    $value = 0;
    if ($bytes > 0) {
      // Generate automatic prefix by bytes
      // If wrong prefix given
      if (!array_key_exists($unit, $units)) {
        $pow = floor(bcdiv(log($bytes), log(1024)));
        $unit = array_search($pow, $units);
      }

      // Calculate byte value by prefix
      $value = (bcdiv($bytes, bcpow(1024, floor($units[$unit]))));
    }

    // If decimals is not numeric or decimals is less than 0
    // then set default value
    if (!is_numeric($decimals) || $decimals < 0) {
      $decimals = 2;
    }

    // Format output
    return sprintf('%.' . $decimals . 'f ' . $unit, $value);
  }
}