<?php
/**
 * @Author SeanJA <http://seanja.com>
 * @license MIT <http://www.opensource.org/licenses/mit-license.php>
 * Just a dummy class so you know what you get back from the smushit compress function
 */
abstract class smushed {

  /**
   * The url of the source image
   * @var string
   */
  public $src;
  /**
   * The size of the file in bytes
   * @var int
   */
  public $src_size;
  /**
   * The url of the smushed image
   * @var string
   */
  public $dest;
  /**
   * The size of the smushed file in bytes
   * @var int
   */
  public $dest_size;
  /**
   * The amount of compression
   * @var float
   */
  public $percent;
  /**
   * Not sure...
   * @var presumably an integer...?
   */
  public $id;
}