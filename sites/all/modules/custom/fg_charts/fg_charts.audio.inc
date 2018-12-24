<?php

/**
 * Get emotions from audio with Empath WebAPI.
 *
 * @param $wav_path
 *  Wave file path.
 */
function fg_charts_empath_emotions($wav_path) {
  $url = 'https://api.webempath.net/v2/analyzeWav';
  $apikey = '9RbnAgzCQWRu35FMGvwNzmL56ZodBaoWCwLzZyb1CzE';
  $path = drupal_realpath($wav_path);

  if (file_exists($path)) {
    $name = drupal_basename($wav_path);
    $mime = mime_content_type($path);
    $wav = new CURLFile($path, $mime, $name);
    $postfields = [
      "apikey" => $apikey,
      "wav" => $wav,
    ];
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $postfields);
    $result = curl_exec($ch);
    $result = json_decode($result);
    print_r($result);
    curl_close($ch);
  }
}