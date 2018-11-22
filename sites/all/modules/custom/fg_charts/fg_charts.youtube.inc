<?php
/**
 * Youtube requests.
 */

/**
 * Save video from youtube.
 *
 * @param $url
 * @param $destination
 *
 * @return bool|null|string
 */
function fg_charts_youtube_video_get($url, $destination) {
  $result = NULL;
  // include_once DRUPAL_ROOT . '/sites/all/libraries/YouTubeDownloader/YouTubeDownloader.php';
  libraries_load('YouTubeDownloader');
  $yt = new YouTubeDownloader();

  $links = $yt->getDownloadLinks($url);

  // Try to get best video resolution.
  $priority = [38, 37, 78, 59, 22, 18];
  $link = '';
  foreach ($priority as $format) {
    if (!empty($links[$format])) {
      $link = $links[$format]['url'];
      break;
    }
  }

  if (!empty($link)) {
    $data = file_get_contents($link);
    if (!empty($data)) {
      $dir = drupal_dirname($destination);
      file_prepare_directory($dir, FILE_CREATE_DIRECTORY|FILE_MODIFY_PERMISSIONS);
      $result = file_unmanaged_save_data($data, $destination);
    }
  }

  return $result;
}
