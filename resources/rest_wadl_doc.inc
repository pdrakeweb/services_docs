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
        'access callback' => 'services_access_menu',
        'access arguments' => array('view'),
        'access arguments append' => TRUE,
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
    'create'   => 'POST', // has no {id} in path
    'retrieve' => 'GET',
    'update'   => 'PUT',
    'delete'   => 'DELETE',
    'index'    => 'GET',
  );

  foreach ($services as $servicename => $service) {
    $wadl['resources'][$count][DOM::ATTRIBUTES]['base'] = $base_url . "/" . $service->path;
    $wadl['resources'][$count]['doc'][0][DOM::ATTRIBUTES]['title'] = $service->title;
    $wadl['resources'][$count]['doc'][0][DOM::CONTENT] = "Doc stuff here!!!";
    // TODO: functionize the creation of a resource?
    // filter resources based on enable
    $resources = array();
    foreach ($service->resources as $resourcename_top => $resource_top) {
      foreach (services_get_resources($servicename) as $resourcename => $resource) {
        if ($resourcename_top == $resource['name']) {
          if ($resourcename_top != "services_docs_rest_wadl") { // filter out WADL?
            $resources[$resourcename] = $resource;
          }
        }
      }
    }
    // Each alias/endpoint
    foreach ($resources as $resourcename => $resource) {
      // Each CRUD operation
      $c_endpoint = array();
      $rud_endpoint = array();
      foreach ($resource['endpoint']['operations'] as $res_operationname => $res_operation) {
        if ($action_mapping[$res_operationname] == "POST") { // POST is a case where it is not requiring a parameter
          $c_endpoint[] = array(
            DOM::ATTRIBUTES => array('name' => "$action_mapping[$res_operationname]"),
          );
        }
        else {
          $rud_endpoint[0]['method'][] = array(
            DOM::ATTRIBUTES => array('name' => "$action_mapping[$res_operationname]"),
          );
          // Only needs to be done once, but is the same for _RUD
          if (in_array($res_operationname, array("retrieve", "update", "delete"))) {
            $rud_endpoint[0][DOM::ATTRIBUTES]['path'] = "{{$resource[$res_operationname]['args'][0]['name']}}";
          }
        }
      }
      $wadl['resources'][$count]['resource'][] = array(
        DOM::ATTRIBUTES => array('path' => $resourcename),
        'resource' => $rud_endpoint,
        'method' => $c_endpoint,
      );
    }
    $count++;

    dpm($resources, "RESOURCES in WADL SERV");
  }
  
  $wadl['output']['now'] = print_r($wadl, TRUE);
  dpm($wadl, "WADL DUMP!");
  return $wadl;
}
