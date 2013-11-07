<?php

/**
 * A multistep form for merging together entities.
 *
 * @param array $form
 * @param array $form_state
 *
 * @return array
 */
function simpleentitymerge_admin_form($form, &$form_state) {
  $form = array();
  $form_state['stage'] = empty($form_state['stage']) ? 'select bundle' : $form_state['stage'];

  switch ($form_state['stage']) {
    case 'select bundle':
      $options = array();
      $entities = field_info_bundles();
      foreach ($entities as $entity => $bundles) {
        $options[$entity] = array();
        foreach ($bundles as $bundle => $info) {
          $options[$entity][$entity . '--' . $bundle] = $bundle;
        }
      }
      $form['entity_bundle_source'] = array(
        '#title' => t('Source'),
        '#type' => 'select',
        '#options' => $options,
      );
      $form['entity_bundle_target'] = array(
        '#title' => t('Target'),
        '#type' => 'select',
        '#options' => $options,
      );

      $form_state['stage'] = 'select instances';
      break;

    case 'select instances':
      if (!empty($form_state['values']['entity_bundle_source']) && !empty($form_state['values']['entity_bundle_target'])) {
        $entity_bundle_source = explode('--', $form_state['values']['entity_bundle_source'], 2);
        $entity_bundle_target = explode('--', $form_state['values']['entity_bundle_target'], 2);
        if (count($entity_bundle_source) === 2 && count($entity_bundle_target) === 2) {
          $form['source'] = array(
            '#type' => 'entityreference',
            '#title' => t('Source'),
            '#era_entity_type' => $entity_bundle_source[0],
            '#era_bundles' => array($entity_bundle_source[1]),
            '#era_cardinality' => 3,
          );

          $form['target'] = array(
            '#type' => 'entityreference',
            '#title' => t('Target'),
            '#era_entity_type' => $entity_bundle_target[0],
            '#era_bundles' => array($entity_bundle_target[1]),
            '#era_cardinality' => 3,
          );

          $form_state['stage'] = 'compare instances';
        }
      }
      break;

    case 'compare instances':
      if (!empty($form_state['values']['source'][0]) && !empty($form_state['values']['target'][0])) {
        $source = $form_state['values']['source'][0];
        $target = $form_state['values']['target'][0];
        $source_entity = entity_load($source['entity_type'], array($source['entity_id']));
        $target_entity = entity_load($target['entity_type'], array($target['entity_id']));
        if (!empty($source_entity[$source['entity_id']]) && !empty($target_entity[$target['entity_id']])) {
          $form['source_type'] = array(
            '#type' => 'hidden',
            '#value' => $source['entity_type'],
          );
          $form['source_id'] = array(
            '#type' => 'hidden',
            '#value' => $source['entity_id'],
          );
          $form['target_type'] = array(
            '#type' => 'hidden',
            '#value' => $target['entity_type'],
          );
          $form['target_id'] = array(
            '#type' => 'hidden',
            '#value' => $target['entity_id'],
          );
          $form['fields'] = array(
            '#tree' => TRUE,
          );

          $field_info_instances = field_info_instances();
          $bundle_fields = $field_info_instances[$target['entity_type']][$target['entity_bundle']];
          foreach ($bundle_fields as $field_name => $field) {
            if (isset($field_info_instances[$target['entity_type']][$target['entity_bundle']][$field_name]) && isset($field_info_instances[$source['entity_type']][$source['entity_bundle']][$field_name])) {
              $source_items = field_get_items($source['entity_type'], $source_entity[$source['entity_id']], $field_name);
              $target_items = field_get_items($target['entity_type'], $target_entity[$target['entity_id']], $field_name);
              $source_view = array();
              $target_view = array();

              // Render the field items so they can be displayed to the user.
              if (is_array($source_items)) {
                foreach ($source_items as $source_item) {
                  $view = field_view_value($source['entity_type'], $source_entity[$source['entity_id']], $field_name, $source_item);
                  if (isset($view['#markup'])) {
                    $source_view[] = array(array('data' => $view['#markup']));
                  }
                }
              }
              if (is_array($target_items)) {
                foreach ($target_items as $target_item) {
                  $view = field_view_value($target['entity_type'], $target_entity[$target['entity_id']], $field_name, $target_item);
                  if (isset($view['#markup'])) {
                    $target_view[] = array(array('data' => $view['#markup']));
                  }
                }
              }

              $form['fields'][$field_name] = array(
                '#type' => 'fieldset',
                '#title' => $field['label'],
                '#collapsible' => TRUE,
                '#collapsed' => FALSE,
              );

              $form['fields'][$field_name]['changes'] = array(
                '#type' => 'item',
                '#title' => $field_name,
                '#markup' => theme('table', array('rows' => $source_view, 'attributes' => array('class' => 'simpleentitymerge'))) . theme('table', array('rows' => $target_view, 'attributes' => array('class' => 'simpleentitymerge'))),
                '#attached' => array(
                  'css' => array(
                    drupal_get_path('module', 'simpleentitymerge') . "/simpleentitymerge.css",
                  ),
                ),
              );

              $form['fields'][$field_name]['doit'] = array(
                '#type' => 'checkbox',
                '#title' => 'Overwrite this field',
                '#default_value' => 0,
              );
            }
          }

          // Show field properties.
          if ($source['entity_type'] === $target['entity_type']) {
            $form['properties'] = array(
              '#type' => 'fieldset',
              '#title' => 'Properties',
              '#collapsible' => TRUE,
              '#collapsed' => TRUE,
              '#tree' => TRUE,
            );

            $entity_properties = entity_get_property_info();
            $allowed_properties = _simpleentitymerge_allowed_entity_properties();
            if (isset($allowed_properties[$source['entity_type']])) {
              foreach ($allowed_properties[$source['entity_type']] as $property) {
                if (in_array($property, array_keys($entity_properties[$source['entity_type']]['properties']))) {
                  $row = array(array(isset($source_entity[$source['entity_id']]->{$property}) ? var_export($source_entity[$source['entity_id']]->{$property}, TRUE) : '', isset($target_entity[$target['entity_id']]->{$property}) ? var_export($target_entity[$target['entity_id']]->{$property}, TRUE) : ''));

                  $form['properties'][$property]['changes'] = array(
                    '#type' => 'item',
                    '#title' => $property,
                    '#markup' => theme('table', array('rows' => $row)),
                  );

                  $form['properties'][$property]['doit'] = array(
                    '#type' => 'checkbox',
                    '#title' => 'Overwrite this property',
                    '#default_value' => 0,
                  );
                }
              }
            }
          }

          $form['update_references'] = array(
            '#type' => 'checkbox',
            '#title' => 'Replace entity references',
            '#default_value' => 1,
          );

          if ($target['entity_type'] === 'user' && $source['entity_type'] === 'user') {
            $form['update_author'] = array(
              '#type' => 'checkbox',
              '#title' => 'Replace authoring info',
              '#default_value' => 1,
            );
          }

          $form['delete_source'] = array(
            '#type' => 'checkbox',
            '#title' => 'Delete the source item',
            '#default_value' => 1,
          );

          $form_state['stage'] = 'overwrite fields';
        }
      }
      break;
  }

  if ($form_state['stage'] !== 'finished') {
    $form['next'] = array(
      '#type' => 'submit',
      '#value' => t('Next'),
    );
  }

  return $form;
}

/**
 * Keep the form from ever properly submitting so we have a multistep form.
 *
 * @param array $form
 * @param array $form_state
 */
function simpleentitymerge_admin_form_submit($form, &$form_state) {
  if ($form_state['stage'] === 'overwrite fields') {
    $source_type = !empty($form_state['values']['source_type']) ? $form_state['values']['source_type'] : false;
    $source_id = !empty($form_state['values']['source_id']) ? $form_state['values']['source_id'] : false;
    $target_type = !empty($form_state['values']['target_type']) ? $form_state['values']['target_type'] : false;
    $target_id = !empty($form_state['values']['target_id']) ? $form_state['values']['target_id'] : false;
    $fields = !empty($form_state['values']['fields']) ? $form_state['values']['fields'] : array();
    $properties = !empty($form_state['values']['properties']) ? $form_state['values']['properties'] : array();

    if ($source_type && $source_id && $target_type && $target_id) {
      $source_entity = entity_load($source_type, array($source_id));
      $target_entity = entity_load($target_type, array($target_id));
      if (!empty($source_entity[$source_id]) && !empty($target_entity[$target_id])) {

        // Filter out which fields the user selected to overwrite.
        foreach ($fields as $field_name => $info) {
          if ($info['doit'] === 0) {
            unset($fields[$field_name]);
          }
        }
        $fields = array_keys($fields);

        // Filter out which properties the user selected to overwrite.
        foreach ($properties as $property_name => $info) {
          if ($info['doit'] === 0) {
            unset($properties[$property_name]);
          }
        }
        $properties = array_keys($properties);

        // Merge the properties and fields.
        $merge_result = _simpleentitymerge_merge_entities($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type, $fields, $properties);
        if ($merge_result !== FALSE) {
          $target_uri = entity_uri($target_type, $target_entity[$target_id]);
          $text = !empty($target_uri['path']) && !empty($target_uri['options']) ? l(t('Complete'), $target_uri['path'], $target_uri['options']) : t('Completed item');
          drupal_set_message($text);

          $batch = array(
            'title' => t('Updating Entities'),
            'operations' => array(),
            'finished' => '_simpleentitymerge_batch_finished',
            'file' => drupal_get_path('module', 'simpleentitymerge') . '/simpleentitymerge.admin.inc',
          );

          // Update entity reference fields.
          if ($form_state['values']['update_references'] === 1) {
            $batch['operations'][] = array('_simpleentitymerge_update_references_field', array($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type));
          }

          // Update authoring information when merging together two users.
          if (isset($form_state['values']['update_author']) && $form_state['values']['update_author'] === 1) {
            $batch['operations'][] = array('_simpleentitymerge_update_author', array($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type));
          }

          // Delete the source entity.
          if ($form_state['values']['delete_source'] === 1) {
            $batch['operations'][] = array('_simpleentitymerge_delete_entity', array($source_type, $source_id));
          }

          if (!empty($batch['operations'])) {
            batch_set($batch);
          }
        }
      }
    }
    $form_state['stage'] = 'finished';
  }
  else {
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Overwrite the fields of the target entity with the source entity.
 *
 * @param stdClass $entity_source
 * @param string $entity_source_type
 * @param stdClass $entity_target
 * @param string $entity_target_type
 * @param array $fields
 */
function _simpleentitymerge_merge_entities($entity_source, $entity_source_type, $entity_target, $entity_target_type, $fields = array(), $properties = array()) {
  // Merge fields.
  $field_info_instances = field_info_instances();
  foreach ($fields as $field) {
    if (isset($field_info_instances[$entity_source_type][$entity_source->type][$field]) && isset($field_info_instances[$entity_target_type][$entity_target->type][$field])) {
      $entity_target->{$field} = $entity_source->{$field};
    }
  }

  // Merge properties.
  if ($entity_source_type === $entity_target_type) {
    $entity_property_info = entity_get_property_info();
    $allowed_properties = _simpleentitymerge_allowed_entity_properties();
    foreach ($properties as $property) {
      if (in_array($property, $allowed_properties[$entity_source_type]) && in_array($property, array_keys($entity_property_info[$entity_source_type]['properties']))) {
        $entity_target->{$property} = $entity_source->{$property};
      }
    }
  }

  // Save the entity.
  try {
    return entity_save($entity_target_type, $entity_target);
  }
  catch (Exception $e) {
    drupal_set_message($e->errorInfo[2], 'error');
    return FALSE;
  }
}

/**
 * Update all entity reference fields across all entities to point to a
 * different entity.
 *
 * @param stdClass $entity_source
 * @param string $entity_source_type
 * @param stdClass $entity_target
 * @param string $entity_target_type
 * @param array $context
 *   A BatchAPI compatible array for storing information on the current state of processing.
 */
function _simpleentitymerge_update_references_field($entity_source, $entity_source_type, $entity_target, $entity_target_type, &$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox']['entities'] = entity_get_info();
    $context['sandbox']['entity_types'] = array_keys($context['sandbox']['entities']);
    $context['sandbox']['entity_types_total'] = count($context['sandbox']['entity_types']);
    $context['sandbox']['current_entity'] = array_shift($context['sandbox']['entity_types']);

    $context['sandbox']['fields'] = array();
    $field_info_fields = field_info_fields();
    foreach ($field_info_fields as $field_name => $field) {
      if ($field['module'] === 'entityreference' && $field['type'] === 'entityreference' && in_array($entity_source_type, array_keys($field['bundles'])) && in_array($entity_target_type, array_keys($field['bundles']))) {
        $context['sandbox']['fields'][$field_name] = $field;
      }
    }
    $context['sandbox']['field_names'] = array_keys($context['sandbox']['fields']);
    $context['sandbox']['fields_total'] = count($context['sandbox']['fields']);
    $context['sandbox']['current_field'] = array_shift($context['sandbox']['field_names']);

    $context['sandbox']['target_id_property'] = $context['sandbox']['entities'][$entity_target_type]['entity keys']['id'];
    $context['sandbox']['source_id_property'] = $context['sandbox']['entities'][$entity_source_type]['entity keys']['id'];

    $context['sandbox']['progress'] = 0;
  }
  $context['finished'] = 1 - ($context['sandbox']['progress'] / ($context['sandbox']['entity_types_total'] * $context['sandbox']['fields_total']));

  if (empty($context['sandbox']['current_entity'])) {
    $context['finished'] = 1;
  }
  else {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', $context['sandbox']['current_entity'])
      ->fieldCondition($context['sandbox']['current_field'], 'target_id', $entity_source->{$context['sandbox']['source_id_property']})
      ->range(0, 5);
    $result = $query->execute();
    if (isset($result[$context['sandbox']['current_entity']]) && count($result[$context['sandbox']['current_entity']]) > 0) {
      $ids = array_keys($result[$context['sandbox']['current_entity']]);
      $items = entity_load($context['sandbox']['current_entity'], $ids);
      foreach ($items as $item) {
        if (isset($item->{$context['sandbox']['current_field']})) {
          foreach ($item->{$context['sandbox']['current_field']} as $language => $value) {
            foreach ($value as $delta => $targets) {
              if (isset($targets['target_id']) && $targets['target_id'] === $entity_source->{$context['sandbox']['source_id_property']}) {
                $item->{$context['sandbox']['current_field']}[$language][$delta]['target_id'] = $entity_target->{$context['sandbox']['target_id_property']};
              }
            }
          }
        }
        try {
          $saved = entity_save($context['sandbox']['current_entity'], $item);
          if ($saved === FALSE) {
            $id = entity_extract_ids($context['sandbox']['current_entity'], $item);
            $sandbox['results'][] = $context['sandbox']['current_entity'] . ' - ' . $id[0];
          }
        }
        catch (Exception $e) {
          $id = entity_extract_ids($context['sandbox']['current_entity'], $item);
          $sandbox['results'][] = $context['sandbox']['current_entity'] . ' - ' . $id[0];
        }
      }
    }
    else {
      // Move on to the next field.
      $context['sandbox']['current_field'] = array_shift($context['sandbox']['field_names']);
      // If all fields have been checked, move on to the next entity and reset
      // the field array.
      if (empty($context['sandbox']['current_field'])) {
        $context['sandbox']['progress']++;
        $context['sandbox']['current_entity'] = array_shift($context['sandbox']['entity_types']);
        $context['sandbox']['field_names'] = array_keys($context['sandbox']['fields']);
        $context['sandbox']['current_field'] = array_shift($context['sandbox']['field_names']);
      }
    }
  }
}

/**
 * Switch the author on all entities from one to another.
 *
 * @param stdClass $entity_source
 * @param string $entity_source_type
 * @param stdClass $entity_target
 * @param string $entity_target_type
 * @param array $context
 *   A BatchAPI compatible array for storing information on the current state of processing.
 */
function _simpleentitymerge_update_author($entity_source, $entity_source_type, $entity_target, $entity_target_type, &$context) {
  if ($entity_source_type === 'user' && $entity_target_type === 'user') {
    if (empty($context['sandbox']['entity_info'])) {
      $context['sandbox']['entity_info'] = entity_get_info();
    }
    if (empty($context['sandbox']['update_author'])) {
      $context['sandbox']['update_author']['uid'] = $context['sandbox']['entity_info']['user']['entity keys']['id'];
      $context['sandbox']['update_author']['entity_types'] = array_keys($context['sandbox']['entity_info']);
      if (($key = array_search('user', $context['sandbox']['update_author']['entity_types'])) !== false) {
        unset($context['sandbox']['update_author']['entity_types'][$key]);
      }
      $context['sandbox']['update_author']['entity_types_total'] = count($context['sandbox']['update_author']['entity_types']);
      $context['sandbox']['update_author']['current_entity'] = array_shift($context['sandbox']['update_author']['entity_types']);
    }

    $context['finished'] = 1 - (count($context['sandbox']['update_author']['entity_types']) / $context['sandbox']['update_author']['entity_types_total']);

    if (!empty($context['sandbox']['update_author']['current_entity']) && !empty($context['sandbox']['update_author']['uid'])) {
      $entity = $context['sandbox']['update_author']['current_entity'];
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', $entity)
        ->propertyCondition($context['sandbox']['update_author']['uid'], $entity_source->{$context['sandbox']['update_author']['uid']})
        ->range(0, 5);
      $result = $query->execute();
      if (isset($result[$entity]) && count($result[$entity]) > 0) {
        $ids = array_keys($result[$entity]);
        $items = entity_load($entity, $ids);
        foreach ($items as $item) {
          if (isset($item->uid)) {
            $item->uid = $entity_target->uid;
            try {
              $saved = entity_save($entity, $item);
              if ($saved === FALSE) {
                $id = entity_extract_ids($entity, $item);
                $sandbox['results'][] = $entity . ' - ' . $id[0];
              }
            }
            catch (Exception $e) {
              $id = entity_extract_ids($entity, $item);
              $sandbox['results'][] = $entity . ' - ' . $id[0];
            }
          }
        }
      }
      else {
        $context['sandbox']['update_author']['current_entity'] = array_shift($context['sandbox']['update_author']['entity_types']);
      }
    }
    else {
      $context['finished'] = 1;
    }
  }
  else {
    $context['finished'] = 1;
  }
}

/**
 * Delete an entity.
 *
 * @param string $source_type
 * @param int $source_id
 * @param array $context
 *   A BatchAPI compatible array for storing information on the current state of processing.
 */
function _simpleentitymerge_delete_entity($source_type, $source_id, &$context) {
  if (!isset($context['message'])) {
    $context['message'] = '';
  }
  try {
    entity_delete($source_type, $source_id);
    $context['message'] .= '<br />' . t('Source entity deleted');
    $context['finished'] = 1;
  }
  catch (Exception $e) {
    $context['message'] .= '<br />' . t('Unable to delete source entity.');
  }
}

/**
 * The list of properties for entities that's allowable to merge.
 */
function _simpleentitymerge_allowed_entity_properties() {
  $list = array(
    'node' => array(
      'uid',
      'title',
      'status',
      'comment',
      'promote',
      'sticky',
      'language',
      'created',
    ),
    'user' => array(
      'name',
      'pass',
      'mail',
      'theme',
      'signature',
      'signature_format',
      'created',
      'status',
      'timezone',
      'language',
      'picture',
      'init',
      'data',
      'roles',
    ),
  );
  drupal_alter('simpleentitymerge_entity_properties', $list);
  return $list;
}

/**
 * Display messages once the batch operation has completed.
 *
 * @param $success
 * @param $results
 * @param $operations
 */
function _simpleentitymerge_batch_finished($success, $results, $operations) {
  if ($success) {
    if (count($results) > 0) {
      $message = count($results) . ' ' . t('were unable to be processed.');
      $message .= theme('item_list', $results);
    }
    else {
      $message = t('Completed');
    }
    drupal_set_message($message);
  }
  else {
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
}
