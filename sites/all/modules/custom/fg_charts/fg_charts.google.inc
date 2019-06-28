<?php
/**
 * Google api requests.
 */

/**
 * Returns an authorized API client.
 *
 * @return Google_Client the authorized client object
 */
function fg_charts_google_client_get() {
  $client = new Google_Client();
  $client->setApplicationName('FGRP');
  $client->setScopes([
    Google_Service_Sheets::SPREADSHEETS,
    Google_Service_Drive::DRIVE_FILE
  ]);

  // Required for Google Client v1.0.0
  // $json = file_get_contents(drupal_realpath('private://credentials.json'));
  // $client->setAuthConfig($json);

  $credentials = variable_get('google_credentials', 0);
  if (!empty($credentials)) {
    $credentials = file_load($credentials);
  }
  if (!empty($credentials)) {
    $credentials_uri = $credentials->uri;
  }
  else {
    $credentials_uri = 'private://credentials.json';
  }
  $client->setAuthConfig(drupal_realpath($credentials_uri));
  $client->setAccessType('offline');
  $client->setPrompt('select_account consent');

  // Load previously authorized token from a file, if it exists.
  // The file token.json stores the user's access and refresh tokens, and is
  // created automatically when the authorization flow completes for the first
  // time.
  $tokenPath = drupal_realpath('private://token.json');
  if (file_exists($tokenPath)) {
    $accessToken = file_get_contents($tokenPath);
    $client->setAccessToken($accessToken);
  }

  // If there is no previous token or it's expired.
  if ($client->isAccessTokenExpired()) {
    // Refresh the token if possible, else fetch a new one.
    if ($client->getRefreshToken()) {
      $client->fetchAccessTokenWithRefreshToken($client->getRefreshToken());
    } else {
      // Request authorization from the user.
      $authUrl = $client->createAuthUrl();
      printf("Open the following link in your browser:\n%s\n", $authUrl);
      print 'Enter verification code: ';
      $authCode = trim(fgets(STDIN));

      // Exchange authorization code for an access token.
      $accessToken = $client->fetchAccessTokenWithAuthCode($authCode);
      $client->setAccessToken($accessToken);

      // Check to see if there was an error.
      if (array_key_exists('error', $accessToken)) {
        throw new Exception(join(', ', $accessToken));
      }
    }
    // Save the token to a file.
    if (!file_exists(dirname($tokenPath))) {
      mkdir(dirname($tokenPath), 0700, true);
    }
    file_put_contents($tokenPath, json_encode($client->getAccessToken()));
  }

  return $client;
}

/**
 * Prepare scene google sheet.
 *
 * @param string $emo_json_path
 *
 * @param string $spreadsheet_id
 *
 * @return array
 */
function fg_charts_scene_google_sheet_prepare($emo_json_path, $spreadsheet_id = NULL) {
  $result = [];

  $limit = variable_get('scene_threshold', 0.7);
  $input_path = drupal_realpath($emo_json_path);
  $data = [];
  $scenes = [];
  if (file_exists($input_path)) {
    $json = file_get_contents($input_path);
    $data = drupal_json_decode($json);
  }
  $datasets = [];
  if (!empty($data['xData']) && !empty($data['datasets'])) {
    foreach ($data['datasets'] as $dataset) {
      if (!in_array($dataset['name'], ['frame', 'timestamp'])) {
        $datasets[$dataset['name']] = array_combine($data['xData'], $dataset['data']);
      }
    }
  }

  $emo_icons = variable_get('emotion_icons', []);

  //  $body = new Google_Service_Sheets_BatchUpdateValuesRequest();
  //
  //  $response = $service->spreadsheets_values->batchUpdate($spreadsheetId, $requestBody);


  // Prints the names and majors of students in a sample spreadsheet:
  // https://docs.google.com/spreadsheets/d/1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms/edit
  //  $spreadsheetId = '1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms';
  //  $range = 'Class Data!A2:E';
  //  $response = $service->spreadsheets_values->get($spreadsheetId, $range);
  //  $values = $response->getValues();

  foreach ($datasets as $name => $data) {
    $scene = ['from' => NULL, 'to' => NULL, 'comment' => $name, 'max_value' => NULL, 'max_time' => NULL];
    $empty_time = FALSE;
    $current_time = 0;
    foreach ($data as $time => $value) {
      if ((int) $time !== $current_time) {
        if (!empty($empty_time) && !empty($scene['from'])) {
          $scene['to'] = $current_time - 1;
          $scenes[$name][] = $scene;
          $scene = ['from' => NULL, 'to' => NULL, 'comment' => $name, 'max_value' => NULL, 'max_time' => NULL];
        }
        $empty_time = TRUE;
      }
      $current_time = (int) $time;
      if ($value >= $limit) {
        $empty_time = FALSE;
        if (empty($scene['from'])) {
          $scene['from'] = (int) $time;
          $scene['max_time'] = $current_time;
          $scene['max_value'] = $value;
        }
        elseif ($value > $scene['max_value']) {
          $scene['max_time'] = $current_time;
          $scene['max_value'] = $value;
        }
      }
    }
  }

  if (!empty($scenes)) {
    libraries_load('google-api-php-client-master');

    // Get the API client and construct the service object.
    try {
      $client = fg_charts_google_client_get();
    }
    catch (Exception $e) {
      watchdog('google', $e->getMessage());

      return [];
    }
    $service = new Google_Service_Sheets($client);
    $drive_service = new Google_Service_Drive($client);

    if (empty($spreadsheet_id)) {
      $spreadsheet = new Google_Service_Sheets_Spreadsheet([
        'properties' => [
          'title' => md5(time()),
        ]
      ]);
      $spreadsheet = $service->spreadsheets->create($spreadsheet, [
        'fields' => 'spreadsheetId'
      ]);

      $requests = [
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'settings'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'happy'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'sad'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'surprise'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'fear'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'anger'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'disgust'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'contempt'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'deleteSheet' => [
            'sheetId' => 0
          ]
        ])
      ];
      // Create two sheets for our pivot table
      $batchUpdateRequest = new Google_Service_Sheets_BatchUpdateSpreadsheetRequest([
        'requests' => $requests
      ]);
      $service->spreadsheets->batchUpdate($spreadsheet->getSpreadsheetId(), $batchUpdateRequest);

      $values = [
        [
          'Setting name',
          'Setting value',
          "Data layer id\n(empty for global settings)"
        ],
        ['count_of_labels', '100', 'happy'],
        ['icon', !empty($emo_icons['happy']) ? $emo_icons['happy'] : '', 'happy'],
        ['color', '#90ed7d', 'happy'],
        ['title', 'Happy', 'happy'],
        ['count_of_labels', '100', 'sad'],
        ['icon', !empty($emo_icons['sad']) ? $emo_icons['sad'] : '', 'sad'],
        ['color', '#f7a35c', 'sad'],
        ['title', 'Sad', 'sad'],
        ['count_of_labels', '100', 'surprise'],
        ['icon', !empty($emo_icons['surprise']) ? $emo_icons['surprise'] : '', 'surprise'],
        ['color', '#8085e9', 'surprise'],
        ['title', 'Surprise', 'surprise'],
        ['count_of_labels', '100',	'fear'],
        ['icon', !empty($emo_icons['fear']) ? $emo_icons['fear'] : '', 'fear'],
        ['color', '#f15c80', 'fear'],
        ['title', 'Fear', 'fear'],
        ['count_of_labels', '100', 'anger'],
        ['icon', !empty($emo_icons['anger']) ? $emo_icons['anger'] : '', 'anger'],
        ['color', '#e4d354', 'anger'],
        ['title', 'Anger', 'anger'],
        ['count_of_labels', '100', 'disgust'],
        ['icon', !empty($emo_icons['disgust']) ? $emo_icons['disgust'] : '', 'disgust'],
        ['color', '#2b908f', 'disgust'],
        ['title', 'Disgust', 'disgust'],
        ['count_of_labels', '100', 'contempt'],
        ['icon', !empty($emo_icons['contempt']) ? $emo_icons['contempt'] : '', 'contempt'],
        ['color', '#f45b5b', 'contempt'],
        ['title', 'Contempt', 'contempt'],
      ];
      $body = new Google_Service_Sheets_ValueRange([
        'values' => $values
      ]);
      $params = [
        'valueInputOption' => "RAW"
      ];
      $service->spreadsheets_values->update($spreadsheet->getSpreadsheetId(), 'settings!A1', $body, $params);

      $newPermission = new Google_Service_Drive_Permission();
      $newPermission->setType('anyone');
      $newPermission->setRole('reader');
      $newPermission->setWithLink(TRUE);
      $drive_service->permissions->create($spreadsheet->getSpreadsheetId(), $newPermission);
    }
    else {
      $spreadsheet = $service->spreadsheets->get($spreadsheet_id);
    }

    foreach ($scenes as $name => $set) {
      $base_name = preg_replace('/(.*)(\_total)$/', '$1', $name);
      $values = [
        ["Started at\n(seconds, required)",	"Finished at\n(second, optional)",	"Comment\n(text, optional)",	"Image\n(link, optional)"],
      ];
      foreach ($set as $scene) {
        $values[] = [
          $scene['from'],
          $scene['to'],
          '',
          '',
        ];

        $values[] = [
          $scene['max_time'],
          '',
          $scene['max_value'],
          !empty($emo_icons[$base_name]) ? $emo_icons[$base_name] : '',
        ];
      }
      $body = new Google_Service_Sheets_ValueRange([
        'values' => $values
      ]);
      $params = [
        'valueInputOption' => "RAW"
      ];
      $service->spreadsheets_values->update($spreadsheet->getSpreadsheetId(), "$base_name!A1",
        $body, $params);
    }
    $response = $drive_service->files->get($spreadsheet->getSpreadsheetId(), ['fields' => ['webViewLink']]);
    $result = [
      'id' => $spreadsheet->getSpreadsheetId(),
      'link' => $response->getWebViewLink(),
    ];
  }

  return $result;
}

/**
 * Prepare scene google sheet all emotions in one row.
 *
 * @param string $emo_json_path
 *
 * @param string $spreadsheet_id
 *
 * @return array
 */
function fg_charts_scene_google_sheet_row_prepare($emo_json_path, $spreadsheet_id = NULL) {
  $result = [];

  $limit = variable_get('scene_threshold', 0.7);
  $input_path = drupal_realpath($emo_json_path);
  $data = [];
  $scenes = [];
  if (file_exists($input_path)) {
    $json = file_get_contents($input_path);
    $data = drupal_json_decode($json);
  }
  $datasets = [];
  if (!empty($data['xData']) && !empty($data['datasets'])) {
    foreach ($data['datasets'] as $dataset) {
      if (!in_array($dataset['name'], ['frame', 'timestamp'])) {
        $datasets[$dataset['name']] = array_combine($data['xData'], $dataset['data']);
      }
    }
  }

  $emo_icons = variable_get('emotion_icons', []);
  $emo_settings = [
    'happy' => [
      'color' => '#f7ed3980',
      'icon' => !empty($emo_icons['happy']) ? $emo_icons['happy'] : '',
      'title' => 'Happy',
    ],
    'sad' => [
      'color' => '#49bfe780',
      'icon' => !empty($emo_icons['sad']) ? $emo_icons['sad'] : '',
      'title' => 'Sad',
    ],
    'surprise' => [
      'color' => '#f79b3980',
      'icon' => !empty($emo_icons['surprise']) ? $emo_icons['surprise'] : '',
      'title' => 'Surprise',
    ],
    'fear' => [
      'color' => '#b8c9d480',
      'icon' => !empty($emo_icons['fear']) ? $emo_icons['fear'] : '',
      'title' => 'Fear',
    ],
    'anger' => [
      'color' => '#da2e1e80',
      'icon' => !empty($emo_icons['anger']) ? $emo_icons['anger'] : '',
      'title' => 'Anger',
    ],
    'disgust' => [
      'color' => '#9ad14b80',
      'icon' => !empty($emo_icons['disgust']) ? $emo_icons['disgust'] : '',
      'title' => 'Disgust',
    ],
    'contempt' => [
      'color' => '#c248e880',
      'icon' => !empty($emo_icons['contempt']) ? $emo_icons['contempt'] : '',
      'title' => 'Contempt',
    ],
  ];

  foreach ($datasets as $name => $data) {
    $scene = ['from' => NULL, 'to' => NULL, 'comment' => $name, 'max_value' => NULL, 'max_time' => NULL];
    $empty_time = FALSE;
    $current_time = 0;
    foreach ($data as $time => $value) {
      if ((int) $time !== $current_time) {
        if (!empty($empty_time) && !empty($scene['from'])) {
          $scene['to'] = $current_time - 1;
          $scenes[$name][] = $scene;
          $scene = ['from' => NULL, 'to' => NULL, 'comment' => $name, 'max_value' => NULL, 'max_time' => NULL];
        }
        $empty_time = TRUE;
      }
      $current_time = (int) $time;
      if ($value >= $limit) {
        $empty_time = FALSE;
        if (empty($scene['from'])) {
          $scene['from'] = (int) $time;
          $scene['max_time'] = $current_time;
          $scene['max_value'] = $value;
        }
        elseif ($value > $scene['max_value']) {
          $scene['max_time'] = $current_time;
          $scene['max_value'] = $value;
        }
      }
    }
  }

  if (!empty($scenes)) {
    libraries_load('google-api-php-client-master');

    // Get the API client and construct the service object.
    try {
      $client = fg_charts_google_client_get();
    }
    catch (Exception $e) {
      watchdog('google', $e->getMessage());

      return [];
    }
    $service = new Google_Service_Sheets($client);
    $drive_service = new Google_Service_Drive($client);

    if (empty($spreadsheet_id)) {
      $spreadsheet = new Google_Service_Sheets_Spreadsheet([
        'properties' => [
          'title' => md5(time()),
        ]
      ]);
      $spreadsheet = $service->spreadsheets->create($spreadsheet, [
        'fields' => 'spreadsheetId'
      ]);

      $requests = [
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'settings'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'addSheet' => [
            'properties' => [
              'title' => 'emotions'
            ]
          ]
        ]),
        new Google_Service_Sheets_Request([
          'deleteSheet' => [
            'sheetId' => 0
          ]
        ])
      ];
      // Create two sheets for our pivot table
      $batchUpdateRequest = new Google_Service_Sheets_BatchUpdateSpreadsheetRequest([
        'requests' => $requests
      ]);
      $service->spreadsheets->batchUpdate($spreadsheet->getSpreadsheetId(), $batchUpdateRequest);

      $values = [
        [
          'Setting name',
          'Setting value',
          "Data layer id\n(empty for global settings)"
        ],
        ['count_of_labels', '100', 'emotions'],
        ['icon', !empty($emo_icons['emotions']) ? $emo_icons['emotions'] : '', 'emotions'],
        ['color', '#afafaf80', 'emotions'],
        ['title', 'Emotions', 'emotions'],
      ];
      $body = new Google_Service_Sheets_ValueRange([
        'values' => $values
      ]);
      $params = [
        'valueInputOption' => "RAW"
      ];
      $service->spreadsheets_values->update($spreadsheet->getSpreadsheetId(), 'settings!A1', $body, $params);

      $newPermission = new Google_Service_Drive_Permission();
      $newPermission->setType('anyone');
      $newPermission->setRole('reader');
      $newPermission->setWithLink(TRUE);
      $drive_service->permissions->create($spreadsheet->getSpreadsheetId(), $newPermission);
    }
    else {
      $spreadsheet = $service->spreadsheets->get($spreadsheet_id);
    }

    $values = [
      [
        "Started at\n(seconds, required)",
        "Finished at\n(second, optional)",
        "Comment\n(text, optional)",
        "Image\n(link, optional)",
        "Scene color\n(optional)",
        "Image position\n(second, optional)",
      ],
    ];
    foreach ($scenes as $name => $set) {
      $base_name = preg_replace('/(.*)(\_total)$/', '$1', $name);
      foreach ($set as $scene) {
        $values[] = [
          $scene['from'],
          $scene['to'],
          '',
          '',
          !empty($emo_settings[$base_name]['color']) ? $emo_settings[$base_name]['color'] : '',
          '',
        ];

        $values[] = [
          $scene['max_time'],
          '',
          $scene['max_value'],
          !empty($emo_settings[$base_name]['icon']) ? $emo_settings[$base_name]['icon'] : '',
          '',
          '',
        ];
      }
    }
    $body = new Google_Service_Sheets_ValueRange([
      'values' => $values
    ]);
    $params = [
      'valueInputOption' => "RAW"
    ];
    $service->spreadsheets_values->update($spreadsheet->getSpreadsheetId(), "emotions!A1",
      $body, $params);

    $response = $drive_service->files->get($spreadsheet->getSpreadsheetId(), ['fields' => ['webViewLink']]);
    $result = [
      'id' => $spreadsheet->getSpreadsheetId(),
      'link' => $response->getWebViewLink(),
    ];
  }

  return $result;
}
