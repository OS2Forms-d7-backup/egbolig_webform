<?php
/**
 * @file
 * egbolig_webform.strongarm.inc
 */

/**
 * Implements hook_strongarm().
 */
function egbolig_webform_strongarm() {
  $export = array();

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'comment_webform';
  $strongarm->value = '0';
  $export['comment_webform'] = $strongarm;

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'node_export_existing';
  $strongarm->value = 'revision';
  $export['node_export_existing'] = $strongarm;

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'soap_webservice';
  $strongarm->value = 'http://egboligws.ballerup.dk/services/service10632.svc?wsdl';
  $export['soap_webservice'] = $strongarm;

  return $export;
}
