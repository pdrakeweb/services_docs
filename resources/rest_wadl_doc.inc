<?php
// TODO: Override: RESTServerViewBuiltIn::render_xml to control XML output to conform to WADL spec
// TODO: Override: RESTServerViewBuiltIn:render_* to always default render_xml b/c wadl makes no sense otherwise...

function services_docs_rest_wadl_resource_definition(){
  // TODO: alter ['endpoint']['postprocess'] to ensure structure? services.runtime.inc:166

  $apci_groups_resource = array(
    'services_docs_rest_wadl' => array(
      'retrieve' => array(
        'file' => array('type' => 'inc', 'module' => 'services_docs', 'name' => 'resources/rest_wadl_doc'),
        'callback' => '_rest_wadl_doc_retrieve',
        'help' => 'Get WADL definition for available services definitions.',
        'access callback' => 'user_access',
        'access arguments' => array('access services documentation'),
        'access arguments append' => FALSE,
      ),
      'index' => array(
        'file' => array('type' => 'inc', 'module' => 'services_docs', 'name' => 'resources/rest_wadl_doc'),
        'callback' => '_rest_wadl_doc_index',
        'help' => 'Debug endpoint',
        'access callback' => 'user_access',
        'access arguments' => array('access services documentation'),
        'access arguments append' => FALSE,
      ),
    ),
  );
  return $apci_groups_resource;
}

function _rest_wadl_doc_retrieve() {
  module_load_include('inc', 'services_docs', 'includes/wadl');
  module_load_include('php', 'services_docs', 'lib/DOM-2.0');

  global $base_url;
  $services = services_endpoint_load_all();
  $wadl = array();
  $count = 0;
  $action_mapping = array(
    'retrieve'         => 'GET',
    'update'           => 'PUT',
    'delete'           => 'DELETE',
    'index'            => 'GET',
    'create'           => 'POST', // has no {id} in path
    'actions'          => 'POST',
    'targeted actions' => 'POST',
    'relationships'    => 'GET',
  );
  // WADL schema
  $wadl[DOM::ATTRIBUTES]["xmlns:wadl"] = "http://wadl.dev.java.net/2009/02"; // Note: Old: http://research.sun.com/wadl/2006/10
  $wadl[DOM::ATTRIBUTES]["xmlns:xsi"] = "http://www.w3.org/2001/XMLSchema-instance";
  $wadl[DOM::ATTRIBUTES]["xmlns:xsd"] = "http://www.w3.org/2001/XMLSchema";
  $wadl[DOM::ATTRIBUTES]["xsi:schemaLocation"] = "http://wadl.dev.java.net/2009/02 wadl.xsd";
  $wadl[DOM::ATTRIBUTES]["xmlns:html"] = "http://www.w3.org/1999/xhtml";

  $wadl[WADL::DOC][0][DOM::ATTRIBUTES]['title'] = variable_get("site_name", "Web Services Application");
  $wadl[WADL::DOC][0][DOM::CONTENT] = variable_get('site_slogan', '');

  foreach ($services as $servicename => $service) { // this pulls in all the endpoints
    if (drupal_match_path($_GET['q'], $service->path . "/*") && array_key_exists("services_docs_rest_wadl", $service->resources)) { // Are we in the current service definition that includes the WADL def
      $wadl[WADL::RESOURCES][$count][DOM::ATTRIBUTES]['base'] = $base_url . "/" . $service->path;
      $wadl[WADL::RESOURCES][$count][WADL::DOC][0][DOM::ATTRIBUTES]['title'] = $service->title;
      $wadl[WADL::RESOURCES][$count][WADL::DOC][0][DOM::CONTENT] = variable_get('services_docs_resource_doc_content', '');
      // TODO: functionize the creation of a resource?
      // filter resources based on enable
      $resources = array();
      foreach ($service->resources as $resourcename_top => $resource_top) { // @todo is this redundant?
        //build an array of resource ops keyed by alias
        foreach (services_get_resources($servicename) as $resourcename => $resource) {
          if ($resourcename_top == $resource['name']) {
            if ($resourcename_top != "services_docs_rest_wadl") { // filter out WADL?
              $resources[$resourcename] = array_intersect_key($resource, array_flip(array('retrieve', 'index', 'create', 'update', 'delete')));
              if (count($relationships = array_intersect_key($resource, array_flip(array('relationships')))) > 0) {
                // Split out the relationships
                foreach ($relationships['relationships'] as $resourcerelationshipsname => $resourcerelationships) {
                  $resources[$resourcename]['relationships_' . $resourcerelationshipsname] = $resourcerelationships;
                }
              }
            }
          }
        }
      }
      // ok we have our array, let's go
      foreach ($resources as $resourcename => $resource){
        $r_info = _build_resource($resourcename, $resource);
        $wadl[WADL::RESOURCES][$count][WADL::RESOURCE][] = $r_info;
      }
      $count++;
    }
  } //wrap this around whe whole thing
  return $wadl;
}
/**
 * Takes a resource key and array, returns an array to WADL
 */
function _build_resource($resourcename, $resource){
  global $user;
  $r_info = array(
    DOM::ATTRIBUTES => array('path' => $resourcename),
  );
  foreach ($resource as $op_top => $def){
    // TODO: Note the loading and access check are straight from services.runtime.inc:117
    // Load the proper file
    if (!empty($def['file']) && $file = $def['file']) {
      module_load_include($file['type'], $file['module'], (isset($file['name']) ? $file['name'] : NULL));
    }

    // Construct access arguments array
    $args = array();
    if (array_key_exists('access arguments', $def)) {
      $access_arguments = $def['access arguments'];
      if (array_key_exists('access arguments append', $def) && $def['access arguments append']) {
        $access_arguments[] = $args;
      }
    }
    else {
      // Just use the arguments array if no access arguments have been specified
      $access_arguments = $args;
    }
    // If the services definition provides an override
    if (array_key_exists('services_docs access arguments', $def)) {
      $access_arguments = $def['services_docs access arguments'];
    }

    if (call_user_func_array($def['access callback'], $access_arguments) || $user->uid == 1) {
      switch ($op_top){
        case 'retrieve':
          $method = _build_retrieve_method($def);
          // @todo make sure that arg[0] comes from path
          if (!isset($r_info[WADL::RESOURCE][$def['args'][0]['name']])){
            $r_info[WADL::RESOURCE][$def['args'][0]['name']] = array(
              DOM::ATTRIBUTES => array('path' => '{'.$def['args'][0]['name'].'}'),
            );
          }
          $r_info[WADL::RESOURCE][$def['args'][0]['name']][WADL::METHOD][] = $method;
        break;
        case 'index':
          $method = _build_retrieve_method($def);
          $r_info[WADL::METHOD][] = $method;
        break;
        case 'create':
          $method = _build_create_method($def);
          $r_info[WADL::METHOD][] = $method;
        break;
        case 'update':
          $method = _build_update_method($def);
          //@todo make sure it comes from path
          if (!isset($r_info[WADL::RESOURCE][$def['args'][0]['name']])){
            $r_info[WADL::RESOURCE][$def['args'][0]['name']] = array(
              DOM::ATTRIBUTES => array('path' => '{'.$def['args'][0]['name'].'}'),
            );
          }
          $r_info[WADL::RESOURCE][$def['args'][0]['name']][WADL::METHOD][] = $method;
        break;
        case 'delete':
          $method = _build_delete_method($def);
          if (!isset($r_info[WADL::RESOURCE][$def['args'][0]['name']])){
            $r_info[WADL::RESOURCE][$def['args'][0]['name']] = array(
              DOM::ATTRIBUTES => array('path' => '{'.$def['args'][0]['name'].'}'),
            );
          }
          $r_info[WADL::RESOURCE][$def['args'][0]['name']][WADL::METHOD][] = $method;
        break;
      }
      if ((strpos($op_top, "relationships_") == 0) && $rel_id = substr_replace($op_top, "", 0, strlen("relationships_"))) {
        $method = _build_retrieve_method($def);
        $r_info[WADL::RESOURCE][$def['args'][0]['name']][WADL::RESOURCE][$rel_id] = array(
        DOM::ATTRIBUTES => array('path' => $rel_id),
        WADL::METHOD => array($method),
        );
      }
    }
  }
  return $r_info;
}

function _build_update_method($def){
  $method = array( //need to inject sub path here
    DOM::ATTRIBUTES => array(
      'name' => 'PUT',
      'id' => $def['callback'] . "-" . $def['args'][0]['name'],
    ),
    WADL::DOC => $def['help'],
  );
  $data_param = array();
  $data_param_count = 0;
  foreach ($def['args'] as $arg){
    if (isset($arg['source']['data'])){
      $data_param[$data_param_count] = array(
        DOM::ATTRIBUTES => array(
          'type' => $arg['type'],
          'required' => !$arg['optional'] ? "true" : "false",
          'style' => 'plain', //@todo this is probably wrong too..
          'name' => $arg['name'],
        ),
        WADL::DOC => $arg['description'],
      );
      if($arg['optional'] == 1){
        if (isset($arg['default value'])){
          $data_param[$data_param_count][DOM::ATTRIBUTES]['default'] = $arg['default value'];
        }
      }
      $data_param_count++;
    }
    // TODO: elseif: isset($arg['source']['param'])
  }

  /* @todo this is probably wrong....*/
  /* they should be pulled in from a parser call, but couldn't find one, there may be one in services
     tools, but this needs research */
  $method[WADL::REQUEST][][WADL::REPRESENTATION] = _build_representations($data_param);
  array_key_exists('response', $def) ? $method[WADL::RESPONSE][] = _build_response($def['response']) : NULL;
  return $method;
}

function _build_retrieve_method($def){
  $method = array( //need to inject sub path here
    DOM::ATTRIBUTES => array(
      'name' => 'GET',
      'id' => $def['callback'] . "-" . $def['args'][0]['name'],
    ),
    WADL::DOC => $def['help'],
  );
  $param = array();
  foreach ($def['args'] as $arg){
    if (isset($arg['source']['param'])){
      $param[] = array(
        DOM::ATTRIBUTES => array(
          'type' => $arg['type'],
          'required' => !$arg['optional'] ? "true" : "false",
          'style' => 'query',
          'name' => $arg['name'],
          'default' => $arg['default value'],
        ),
        WADL::DOC => $arg['description'],
      );
    }
  }
  $method[WADL::REQUEST][][WADL::PARAM] = $param;
  array_key_exists('response', $def) ? $method[WADL::RESPONSE][] = _build_response($def['response']) : NULL;
  return $method;
}

function _build_delete_method($def){
  $method = array(
    DOM::ATTRIBUTES => array(
      'name' => 'DELETE',
      'id' => $def['callback'] . "-" . $def['args'][0]['name'],
    ),
    WADL::DOC => $def['help'],
  );
  $param = array();
  foreach ($def['args'] as $arg){
    if (isset($arg['source']['param'])){
      $param[] = array(
        DOM::ATTRIBUTES => array(
          'type' => $arg['type'],
          'required' => !$arg['optional'] ? "true" : "false",
          'style' => 'query',
          'name' => $arg['name'],
          'default' => $arg['default value'],
        ),
        WADL::DOC => $arg['description'],
      );
    }
  }
  $method[WADL::REQUEST][][WADL::PARAM] = $param;
  array_key_exists('response', $def) ? $method[WADL::RESPONSE][] = _build_response($def['response']) : NULL;
  return $method;
}

// TODO: well this function needs some love... this should be in a utility class
function flatten_array($array){
  if (!$array) return false;
   $flat = array();
   $RII = new RecursiveIteratorIterator(new RecursiveArrayIterator($array));
   foreach ($RII as $value) $flat[$RII::key] = $value;
   return $flat;
}

function _build_create_method($def){
  $method = array( //need to inject sub path here
    DOM::ATTRIBUTES => array(
      'name' => 'POST',
      'id' => $def['callback'] . "-" . 'id',
    ),
    WADL::DOC => $def['help'],
  );
  $param = array();
  $param_count = 0;
  foreach ($def['args'] as $arg){
    if (isset($arg['source']['data'])){
      $param[$param_count] = array(
        DOM::ATTRIBUTES => array(
          'type' => $arg['type'],
          'required' => !$arg['optional'] ? "true" : "false",
          'style' => 'query', //@todo this is probably wrong too..
          'name' => $arg['name'],
        ),
        WADL::DOC => $arg['description'],
      );
      if($arg['optional'] == 1){
        if (isset($arg['default value'])){
          $param[$param_count][DOM::ATTRIBUTES]['default'] = $arg['default value'];
        }
      }
      $param_count++;
    }
  }
  /* @todo this is probably wrong....*/
  /* they should be pulled in from a parser call, but couldn't find one, there may be one in services
     tools, but this needs research */
  $method[WADL::REQUEST][][WADL::REPRESENTATION] = _build_representations($param);
  array_key_exists('response', $def) ? $method[WADL::RESPONSE][] = _build_response($def['response']) : NULL;
  return $method;
}

function _build_response($response) {
  return array(
    WADL::REPRESENTATION => array(array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/json'),
      WADL::DOC => json_encode(array_keys($response)),
    )),
  );
}

function _build_representations($param){
  // TODO: Pull representations from Services module api directly
  $return = array(
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/x-www-form-urlencoded'),
      WADL::PARAM => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/x-yaml'),
      WADL::PARAM => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/json'),
      WADL::PARAM => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/vnd.php.serialized'),
      WADL::PARAM => $param,
    ),
  );
  return $return;
}
