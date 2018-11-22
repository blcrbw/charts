<?php
/**
 * Protobrain requests.
 */

/**
 * Get request method by type and data
 *
 * @param $type
 * @param $data
 *
 * @return bool|string
 */
function protobrain_integration_get_method($type, $data) {
  $method = FALSE;

  if ($type == 'send') {
    if (empty($data['id'])) {
      $method = 'POST';
    }
    else {
      $method = 'PUT';
    }
  }
  if ($type == 'delete') {
    if (!empty($data['id'])) {
      $method = 'DELETE';
    }
  }

  return $method;
}

/**
 * Send request to ProtoBrain.
 *
 * @param string $request
 *  Type of request, i.e. 'act', 'stream', e.t.c.
 * @param array $data
 *  Data array.
 * @param string $method
 *  Http method.
 *
 * @return array
 *  Decoded response data.
 */
function protobrain_integration_request_send($request, $data, $method = 'POST', $ignore_async = FALSE) {
  $api_url = variable_get('protobrain_integration_api_url', 'http://api.platform.protobrain.qajedi.ru');
  $api_prefix = variable_get('protobrain_integration_api_prefix', 'api/v1');
  // @TODO: make to be able to get different api keys.
  $api_key = variable_get('protobrain_integration_api_key', FALSE);
  if (empty($api_key)) {
    watchdog('protobrain', 'Api key for protobrain request is not set.');

    return [];
  }

  $client_token_info = variable_get('protobrain_integration_client_token_info', []);
  if (empty($client_token_info)) {
    $client_id = variable_get('protobrain_integration_client_id', '');
    $client_secret = variable_get('protobrain_integration_client_secret', '');
    $token_request_url = "$api_url/$api_prefix/auth/token";
    $token_data = [
      'client_id' => $client_id,
      'client_secret' => $client_secret,
      'grant_type' => 'client_credentials',
    ];
    $token_options = [
      'method' => 'POST',
      'headers' => [
        'Content-Type' => 'application/x-www-form-urlencoded',
        'X-Api-Key' => $api_key,
      ],
      'data' => drupal_http_build_query($token_data),
    ];
    try {
      $token_response = drupal_http_request($token_request_url, $token_options);
    }
    catch (Exception $e) {
      $token_response = new stdClass();
      $token_response->error = $e->getMessage();
    }
    if (isset($token_response->error)) {
      watchdog('protobrain', 'Protobrain client authorization request failed with: ' . $token_response->error);
    }
    else {
      $client_token_info = !empty($token_response->data) ? drupal_json_decode($token_response->data) : [];
      if (!empty($client_token_info)) {
        variable_set('protobrain_integration_client_token_info', $client_token_info);
      }
    }
  }
  $request_url = "$api_url/$api_prefix/$request";
  if (in_array($method, ['PUT', 'DELETE', 'GET']) && !empty($data['id'])) {
    $request_url .= '/' . $data['id'];
  }

  $options = [
    'method' => $method,
    'headers' => [
      'Content-Type' => 'application/json',
      'X-Api-Key' => $api_key,
      'X-Api-Fingerprint' => 'FocusGroup',
    ],
    'timeout' => 15,
  ];

  if ($workspace_id = variable_get('protobrain_integration_fire_workspace_id', FALSE)) {
    $options['headers']['X-Api-Workspace-Id'] = $workspace_id;
  }
  if (!empty($data)) {
    if ($method === 'GET') {
      $request_url .= '?' . drupal_http_build_query($data);
    }
    else {
      $options['data'] = drupal_json_encode($data);
    }
  }
  if (!empty($client_token_info['access_token'])) {
    $options['headers']['Authorization'] = 'Bearer ' . $client_token_info['access_token'];
  }
  if (!empty($ignore_async)) {
    $options['headers']['X-Api-Ignore-Async'] = TRUE;
  }

  try {
    $response = drupal_http_request($request_url, $options);
  }
  catch (Exception $e) {
    $response = new stdClass();
    $response->error = $e->getMessage();
  }
  if (variable_get('api_debug_enabled', FALSE)) {
    $debug_data = [
      'url' => $request_url,
      'options' => $options,
      'response' => $response,
    ];
    watchdog('api_debug_protobrain', drupal_json_encode($debug_data));
  }
  if (isset($response->error)) {
    // @TODO: catch expired token error.
    watchdog('protobrain', 'Protobrain request "' . $request . '" failed with: ' . $response->error);
    $result_data = [];
  }
  elseif (!empty($response->data)) {
    $result_data = drupal_json_decode($response->data);
    if (!empty($result_data['error'])) {
      watchdog('protobrain', 'Protobrain request "' . $request . '" return error: ' . $result_data['error']['message']);
      $result_data = [];
    }
  }
  else {
    $result_data = [];
  }

  return $result_data;
}

/**
 * Get protobrain act for video item.
 */
function protobrain_integration_video_act_import($node) {
  if (!empty($node->field_url[LANGUAGE_NONE][0]['value'])
    && valid_url($node->field_url[LANGUAGE_NONE][0]['value'], TRUE)
    && empty($node->field_protobrain_act_id[LANGUAGE_NONE][0]['value'])
  ) {
    $act = protobrain_integration_act_import($node->field_url[LANGUAGE_NONE][0]['value']);
    if (!empty($act['act']['id'])) {
      $node->field_protobrain_act_id[LANGUAGE_NONE][0]['value'] = $act['act']['id'];
      if (!empty($act['act']['url'])) {
        $node->field_fireto_url[LANGUAGE_NONE][0]['url'] = $act['act']['url'];
      }
      field_attach_update('node', $node);
    }
  }
}

/**
 * Get act by video url.
 *
 * @param string $url
 *  Url string.
 *
 * @return array
 *  Act data array.
 */
function protobrain_integration_act_import($url) {
  return protobrain_integration_request_send('act/import', ['url' => $url], 'PUT');
}

/**
 * Add or update fire layer.
 *
 * @param int $act_id
 *  Act id.
 * @param string $url
 *  Google spreadsheet url string.
 * @param int $fire_layer_id
 *  Fire layer id.
 *
 * @return array
 *  Act data array.
 */
function protobrain_integration_fire_layer_add($act_id, $url, $fire_layer_id = NULL) {
  $data = ['act_id' => $act_id, 'google_doc_link' => $url];
  if (!empty($fire_layer_id)) {
    $data['id'] = $fire_layer_id;
  }

  return protobrain_integration_request_send('fire/fire-layer', $data, protobrain_integration_get_method('send', $data));
}

/**
 * Delete fire layer.
 *
 * @param int $fire_layer_id
 *  Fire layer id.
 *
 * @return array
 *  Act data array.
 */
function protobrain_integration_fire_layer_delete($fire_layer_id) {
  $data['id'] = $fire_layer_id;

  return protobrain_integration_request_send('fire/fire-layer', $data, protobrain_integration_get_method('delete', $data));
}

