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
                '#default_value' => (!empty($source_view)) ? 1 : 0,
              );
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

    case 'overwrite fields':
      $source_type = !empty($form_state['values']['source_type']) ? $form_state['values']['source_type'] : false;
      $source_id = !empty($form_state['values']['source_id']) ? $form_state['values']['source_id'] : false;
      $target_type = !empty($form_state['values']['target_type']) ? $form_state['values']['target_type'] : false;
      $target_id = !empty($form_state['values']['target_id']) ? $form_state['values']['target_id'] : false;
      $fields = !empty($form_state['values']['fields']) ? $form_state['values']['fields'] : array();
      $form['finished'] = array(
        '#type' => 'item',
        '#title' => 'Completed',
        '#markup' => t('Sorry, there was a problem'),
      );
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

          // Merge the fields.
          _simpleentitymerge_merge_entities($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type, $fields);
          $target_uri = entity_uri($target_type, $target_entity[$target_id]);
          $text = !empty($target_uri['path']) && !empty($target_uri['options']) ? l(t('Complete'), $target_uri['path'], $target_uri['options']) : t('Completed item');
          $form['finished']['#markup'] = $text;

          // Update entity reference fields.
          if ($form_state['values']['update_references'] === 1) {
            $context = array(
              'finished' => 0,
            );
            while ($context['finished'] !== 1) {
              _simpleentitymerge_update_references_field($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type, $context);
            }
          }

          // Update authoring information when merging together two users.
          if (isset($form_state['values']['update_author']) && $form_state['values']['update_author'] === 1) {
            $context = array(
              'finished' => 0,
            );
            _simpleentitymerge_update_author($source_entity[$source_id], $source_type, $target_entity[$target_id], $target_type, $context);
          }

          // Delete the source entity.
          if ($form_state['values']['delete_source'] === 1) {
            try {
              entity_delete($source_type, $source_id);
            }
            catch (Exception $e) {
              $form['finished']['#markup'] .= '<br />' . t('The source was not deleted');
            }
          }
        }
      }
      $form['finished']['#markup'] .= '<br /><br />' . l(t('Merge another'), 'admin/simpleentitymerge');
      $form_state['stage'] = 'finished';
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
  $form_state['rebuild'] = TRUE;
}

function _simpleentitymerge_entity_id($entity_type) {
  switch ($entity_type) {
    case 'node':
      return 'nid';
    case 'user':
      return 'uid';
    case 'taxonomy_term':
      return 'tid';
    case 'comment':
      return 'cid';
  }
  $other_ids = module_invoke_all('simpleentitymerge_ids');
  if (!empty($other_ids[$entity_type])) {
    return $other_ids[$entity_type];
  }

  return FALSE;
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
function _simpleentitymerge_merge_entities($entity_source, $entity_source_type, $entity_target, $entity_target_type, $fields) {
  $field_info_instances = field_info_instances();
  foreach ($fields as $field) {
    if (isset($field_info_instances[$entity_source_type][$entity_source->type][$field]) && isset($field_info_instances[$entity_target_type][$entity_target->type][$field])) {
      $entity_target->$field = $entity_source->$field;
    }
  }
  try {
    return entity_save($entity_target_type, $entity_target);
  }
  catch (Exception $e) {
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
    $field_info_fields = field_info_fields();
    $context['sandbox']['entities'] = entity_get_info();
    foreach ($field_info_fields as $field_name => $field) {
      if ($field['module'] === 'entityreference' && $field['type'] === 'entityreference') {
        $context['sandbox']['fields'][$field_name] = $field;
      }
    }
    $context['sandbox']['current_field'] = array_shift($context['sandbox']['fields']);
  }

  if (!$context['sandbox']['current_field']) {
    $context['finished'] = 1;
  }
  else {
    foreach ($context['sandbox']['entities'] as $entity => $entity_info) {
      $target_id_property = _simpleentitymerge_entity_id($entity_target_type);
      $source_id_property = _simpleentitymerge_entity_id($entity_source_type);
      if ($target_id_property && $source_id_property) {
        $query = new EntityFieldQuery();
        $query->entityCondition('entity_type', $entity)
          ->fieldCondition($context['sandbox']['current_field']['field_name'], 'target_id', $entity_source->{$source_id_property});
        $result = $query->execute();
        if (isset($result[$entity]) && count($result[$entity]) > 0) {
          $ids = array_keys($result[$entity]);
          $items = entity_load($entity, $ids);
          foreach ($items as $item) {
            if (isset($item->{$context['sandbox']['current_field']['field_name']})) {
              foreach ($item->{$context['sandbox']['current_field']['field_name']} as $language => $value) {
                foreach ($value as $delta => $targets) {
                  if (isset($targets['target_id']) && $targets['target_id'] === $entity_source->{$source_id_property}) {
                    $item->{$context['sandbox']['current_field']['field_name']}[$language][$delta]['target_id'] = $entity_target->{$target_id_property};
                  }
                }
              }
            }
            try {
              entity_save($entity, $item);
            }
            catch (Exception $e) {}
          }
        }
      }
    }
    $context['sandbox']['current_field'] = array_shift($context['sandbox']['fields']);
  }
}

function _simpleentitymerge_update_author($entity_source, $entity_source_type, $entity_target, $entity_target_type, &$context) {
  if ($entity_source_type === 'user' && $entity_target_type === 'user') {
    if (empty($context['sandbox'])) {
      $context['sandbox']['entities'] = entity_get_info();
      unset($context['sandbox']['entities']['user']);
    }
    foreach ($context['sandbox']['entities'] as $entity => $entity_info) {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', $entity)
        ->propertyCondition('uid', $entity_source->uid);
      $result = $query->execute();
      $result = $query->execute();
      if (isset($result[$entity]) && count($result[$entity]) > 0) {
        $ids = array_keys($result[$entity]);
        $items = entity_load($entity, $ids);
        foreach ($items as $item) {
          $item->uid = $entity_target->uid;
          try {
            entity_save($entity, $item);
          }
          catch (Exception $e) {}
        }
      }
    }
    $context['finished'] = 1;
  }
  else {
    $context['finished'] = 1;
  }
}
