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
  $strongarm->name = 'soap_webservice';
  $strongarm->value = 'http://egboligws.ballerup.dk/services/service10632.svc?wsdl';
  $export['soap_webservice'] = $strongarm;

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'webform_email_html_capable';
  $strongarm->value = 1;
  $export['webform_email_html_capable'] = $strongarm;

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'webform_email_html_capable,';
  $strongarm->value = 1;
  $export['webform_email_html_capable,'] = $strongarm;

  return $export;
}
