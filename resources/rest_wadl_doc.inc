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
  $wadl[DOM::ATTRIBUTES]["xmlns"] = "http://research.sun.com/wadl/2006/10";
  $wadl[DOM::ATTRIBUTES]["xmlns:xsi"] = "http://www.w3.org/2001/XMLSchema-instance";
  $wadl[DOM::ATTRIBUTES]["xmlns:xsd"] = "http://www.w3.org/2001/XMLSchema";
  $wadl[DOM::ATTRIBUTES]["xsi:schemaLocation"] = "http://wadl.dev.java.net/2009/02 wadl.xsd";
  $wadl[DOM::ATTRIBUTES]["xmlns:html"] = "http://www.w3.org/1999/xhtml";
  foreach ($services as $servicename => $service) { // this pulls in all the endpoints
    $wadl['resources'][$count][DOM::ATTRIBUTES]['base'] = $base_url . "/" . $service->path;
    $wadl['resources'][$count]['doc'][0][DOM::ATTRIBUTES]['title'] = $service->title;
    $wadl['resources'][$count]['doc'][0][DOM::CONTENT] = "Doc stuff here!!!";
    // TODO: functionize the creation of a resource?
    // filter resources based on enable
    $resources = array();
    foreach ($service->resources as $resourcename_top => $resource_top) { // @todo is this redundant?
      //build an array of resource ops keyed by alias
      foreach (services_get_resources($servicename) as $resourcename => $resource) {
        if ($resourcename_top == $resource['name']) {
          if ($resourcename_top != "services_docs_rest_wadl") { // filter out WADL?
            $resources[$resourcename] = $resource;
          }
        }
      }
    }
  } //wrap this around whe whole thing
  // ok we have our array, let's go
  foreach ($resources as $resourcename => $resource){
    $r_info = _build_resource($resourcename, $resource);
    $wadl['resources'][$count]['resource'][] = $r_info;
    //$count ++;
    /*$wadl['resources'][]['resource'][] = array(
        DOM::ATTRIBUTES => array('path' => $resourcename),
        'resource' => $r_info['rud'],
        //'method' => $c_endpoint,
    );*/
  }
  return $wadl;
}
/**
 * Takes a resource key and array, returns an array to WADL
 */
function _build_resource($resourcename, $resource){
  $r_info = array(
    DOM::ATTRIBUTES => array('path' => $resourcename),
  );
  //$ops_count = 0;
  foreach ($resource as $op_top => $def){
    switch ($op_top){
      case 'retrieve':
        $method = _build_retrieve_method($def);
       //$r_info['resource'][]['method'][] = $method;
       // @todo make sure that arg[0] comes from path
       if (!isset($r_info['resource'][$def['args'][0]['name']])){
          $r_info['resource'][$def['args'][0]['name']] = array(
            DOM::ATTRIBUTES => array('path' => '{'.$def['args'][0]['name'].'}'),
          );
       }
        $r_info['resource'][$def['args'][0]['name']]['method'][] = $method;
      break;
      case 'index':
        $method = _build_retrieve_method($def);
        $r_info['method'][] = $method;
      break;
      case 'relationships':
        foreach($def as $rel_id => $rel){
          $method = _build_retrieve_method($rel);
          $r_info['resource'][$rel['args'][0]['name']]['resource'][$rel_id] = array(
            DOM::ATTRIBUTES => array('path' => $rel_id),
            'method' => array($method),
          );
        }
      break;
      case 'create':
        $method = _build_create_method($def);
        $r_info['method'][] = $method;
      break;
      case 'update':
        //$method = $wadl;
        $method = _build_update_method($def);
        //@todo make sure it comes from path
        if (!isset($r_info['resource'][$def['args'][0]['name']])){
          $r_info['resource'][$def['args'][0]['name']] = array(
            DOM::ATTRIBUTES => array('path' => '{'.$def['args'][0]['name'].'}'),
          );
        }
        $r_info['resource'][$def['args'][0]['name']]['method'][] = $method;
      break;
    }
    //$ops_count++;
  }
  return $r_info;
}

function _build_update_method($def){
  $method = array( //need to inject sub path here
    DOM::ATTRIBUTES => array(
      'name' => 'PUT',
      'id' => $def['args'][0]['name'],
    ),
    'doc' => $def['help'],
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
        'doc' => $arg['description'],
      );
      if($arg['optional'] == 1){
        if (isset($arg['default value'])){
          $data_param[$data_param_count][DOM::ATTRIBUTES]['default'] = $arg['default value'];
        }
      }
      $data_param_count++;
    }
    /*else if (isset($arg['source']['param'])){
      
    }*/
  }
    
    /* @todo this is probably wrong....*/
  /*they should be pulled in from a parser call, but couldn't find one, there may be one in services
   tools, but this needs research*/
  $method['request'][]['representation'] = _build_representations($data_param);
  return $method;
}

function _build_retrieve_method($def){
  $method = array( //need to inject sub path here
    DOM::ATTRIBUTES => array(
      'name' => 'GET',
      'id' => $def['args'][0]['name'],
    ),
    'doc' => $def['help'],
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
        'doc' => $arg['description'],
      );
    }
  }
  $method['request'][]['param'] = $param;
  //module_load_include($def['file']['type'], $def['file']['module'], $def['file']['name']);
  //$method['response'][]['representation'] = _build_representations($param);
  //build the response
  //$response = flatten_array($def['response']['data']);
  //$method['response'][]['representation']
  return $method;
}

//well this function needs some love
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
      'id' => 'id',
    ),
    'doc' => $def['help'],
  );
  $param = array();
  $param_count = 0;
  foreach ($def['args'] as $arg){
    if (isset($arg['source']['data'])){
      $param[$param_count] = array(
        DOM::ATTRIBUTES => array(
          'type' => $arg['type'],
          'required' => !$arg['optional'] ? "true" : "false",
          'style' => 'plain', //@todo this is probably wrong too..
          'name' => $arg['name'],
        ),
        'doc' => $arg['description'],
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
  /*they should be pulled in from a parser call, but couldn't find one, there may be one in services
   tools, but this needs research*/
  $method['request'][]['representation'] = _build_representations($param);
  return $method;
}

function _build_representations($param){
  $return = array(
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/x-www-form-urlencoded'),
      'param' => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/x-yaml'),
      'param' => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/json'),
      'param' => $param,
    ),
    array(
      DOM::ATTRIBUTES => array('mediaType' => 'application/vnd.php.serialized'),
      'param' => $param,
    ),
  );
  return $return;
}

    // Each alias/endpoint
    /*foreach ($resources as $resourcename => $resource) {
      // Each CRUD operation
      $c_endpoint = array();
      $rud_endpoint = array();
      
      foreach ($resource['endpoint']['operations'] as $res_operationname => $res_operation) {
        // Only needs to be done once, but is the same for _RUD
        // @todo path should really be getting pulled from path parameter in operation
        if (in_array($res_operationname, array("retrieve", "update", "delete"))) {
          $rud_endpoint_idopname = $resource[$res_operationname]['args'][0]['name'];
          $rud_endpoint[0][DOM::ATTRIBUTES]['path'] = "{{$rud_endpoint_idopname}}";
        }
      }
      foreach ($resource['endpoint']['operations'] as $res_operationname => $res_operation) {
        $current = array();
        $current[DOM::ATTRIBUTES]['name'] = "$action_mapping[$res_operationname]";
        $params = array();
        foreach ($resource[$res_operationname]['args'] as $resource_args_param) {
          if ($resource_args_param['source']['param']){ //list query parameters
          $params[] = array(
            DOM::ATTRIBUTES => array(
              'type' => $resource_args_param['type'],
              'required' => !$resource_args_param['optional'] ? "true" : "false",
              'style' => 'query',
              'name' => $resource_args_param['name'],
              'default' => $resource_args_param['default value'],
            ),
           'doc' => $resource_args_param['description'],
          );
          }
        }
        $current['request'][]['param'] = $params; // @todo - this needs to change in order for the params to come through
        //$current['response'] = NULL;
        $current['doc'] = $resource[$res_operationname]['help'];
        if ($action_mapping[$res_operationname] == "POST") { // POST is a case where it is not requiring a parameter
          $c_endpoint[] = $current;
        }
        else {
          if (isset($rud_endpoint_idopname)) {
            $current[DOM::ATTRIBUTES]['id'] = $rud_endpoint_idopname;
          }
          $rud_endpoint[0]['method'][] = $current;
        }
      }
      $wadl['resources'][$count]['resource'][] = array(
        DOM::ATTRIBUTES => array('path' => $resourcename),
        'resource' => $rud_endpoint,
        'method' => $c_endpoint,
      );
      unset($rud_endpoint_idopname);
    }
    $count++;
  }
  
  return $wadl;
}*/
    