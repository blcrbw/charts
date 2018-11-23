<?php
/**
 * Form callbacks.
 */

/**
 * Form builder for fg_chars_settings form.
 */
function fg_charts_settings_form($form, $form_state) {
  $form = [];

  $form['api_debug_enabled'] = [
    '#type' => 'checkbox',
    '#title' => t('Enable api debug'),
    '#default_value' => variable_get('api_debug_enabled', FALSE),
  ];
  $form['openface_command'] = [
    '#type' => 'textfield',
    '#title' => t('Openface command'),
    '#default_value' => variable_get('openface_command', '/home/blc/prj/OpenFace/build/bin/FeatureExtraction -f !video -out_dir !out_dir'),
  ];

  $emotions = [
    'happy',
    'sad',
    'surprise',
    'fear',
    'anger',
    'disgust',
    'contempt',
  ];
  $default_icons = variable_get('emotion_icons', []);
  $form['emotion_icons'] = [
    '#type' => 'fieldset',
    '#title' => 'Emotion icons',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  ];
  foreach ($emotions as $emotion) {
    $form['emotion_icons'][$emotion] = [
      '#type' => 'textfield',
      '#title' => ucfirst($emotion),
      '#default_value' => !empty($default_icons[$emotion]) ? $default_icons[$emotion] : '',
    ];
  }

  $form['protobrain_integration'] = [
    '#type' => 'fieldset',
    '#title' => 'Protobrain settings',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => FALSE,
  ];
  $form['protobrain_integration']['protobrain_integration_multistream_enabled'] = [
    '#type' => 'checkbox',
    '#title' => t('Enable multiple ProtoBrain streams'),
    '#default_value' => variable_get('protobrain_integration_multistream_enabled'),
  ];
  $form['protobrain_integration']['protobrain_integration_client_secret'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain client secret'),
    '#default_value' => variable_get('protobrain_integration_client_secret'),
  ];
  $form['protobrain_integration']['protobrain_integration_client_id'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain client id'),
    '#default_value' => variable_get('protobrain_integration_client_id'),
  ];
  $form['protobrain_integration']['protobrain_integration_api_url'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain api url'),
    '#default_value' => variable_get('protobrain_integration_api_url'),
  ];
  $form['protobrain_integration']['protobrain_integration_api_prefix'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain api prefix'),
    '#default_value' => variable_get('protobrain_integration_api_prefix', 'api/v1'),
  ];
  $form['protobrain_integration']['protobrain_integration_fire_url'] = [
    '#type' => 'textfield',
    '#title' => t('Fire url'),
    '#default_value' => variable_get('protobrain_integration_fire_url'),
  ];
  $form['protobrain_integration']['protobrain_integration_fire_workspace_id'] = [
    '#type' => 'textfield',
    '#title' => t('Fire workspace id'),
    '#default_value' => variable_get('protobrain_integration_fire_workspace_id'),
  ];
  $form['protobrain_integration']['protobrain_integration_media_source_id'] = [
    '#type' => 'textfield',
    '#title' => t('Media source id'),
    '#default_value' => variable_get('protobrain_integration_media_source_id'),
  ];
  $form['protobrain_integration']['protobrain_integration_thermometer_url'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain thermometer request url'),
    '#default_value' => variable_get('protobrain_integration_thermometer_url'),
  ];
  $form['protobrain_integration']['protobrain_integration_api_key'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain api key'),
    '#default_value' => variable_get('protobrain_integration_api_key'),
  ];
  $form['protobrain_integration']['protobrain_integration_solution_id'] = [
    '#type' => 'textfield',
    '#title' => t('ProtoBrain solution id'),
    '#default_value' => variable_get('protobrain_integration_solution_id'),
  ];

  $form['google_credentials'] = [
    '#type' => 'managed_file',
    '#title' => t('Google credentials'),
    '#upload_validators' => [
      'file_validate_extensions' => ['txt json'],
    ],
    '#upload_location' => 'private://google/',
    '#default_value' => variable_get('google_credentials', 0),
  ];

  $form['au_threshold'] = [
    '#type' => 'textfield',
    '#title' => t('AU threshold'),
    '#default_value' => variable_get('au_threshold', 0.5),
  ];
  $form['scene_threshold'] = [
    '#type' => 'textfield',
    '#title' => t('Scene threshold'),
    '#default_value' => variable_get('scene_threshold', 0.7),
  ];

  $form = system_settings_form($form);
  array_unshift($form['#submit'], 'fg_charts_settings_form_submit');

  return $form;
}


/**
 * Form submit for fg_charts_settings_form.
 */
function fg_charts_settings_form_submit($form, &$form_state) {
  $files = [
    'google_credentials'
  ];

  foreach ($files as $file) {
    $new_file_fid = $form_state['values'][$file];
    $current_file_fid = variable_get($file, 0);
    // Check if file was changed.
    if ($new_file_fid !== $current_file_fid) {
      // Try to load new offer file.
      if ($new_file_file = file_load($new_file_fid)) {
        // Resave new offer file with permanent status and filled usages.
        $new_file_file->status = FILE_STATUS_PERMANENT;
        file_save($new_file_file);
        file_usage_add($new_file_file, 'core', "site_settings_{$file}_file", 1);
      }
      // Try to remove old offer file.
      if ($current_file_file = file_load($current_file_fid)) {
        // Drop own usages.
        file_usage_delete($current_file_file, 'core', "site_settings_{$file}_file", 1);
        // Completely remove file.
        file_delete($current_file_file);
      }
    }
  }
}