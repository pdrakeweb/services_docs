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
  $wadl = array(
    "resources" => array(
      "doc" => "docs here?",
      "resource" => array(
        "path" => "pathinfo1",
        "param" => array(
          "name" => "parameter name",
          "doc"  => "param doc info?",
        ),
        "resource" => array(
          "path" => "pathinfo2",
        )
      ),
      "resource" => array(
        "path" => "pathinfo3",
        "doc"  => "can you put doc info here?",
      )
    ),
    "book" => array(
      0 => "one",
      1 => "two",
      2 => "three",
    ),
  );
  
  $wadl = array();
  $wadl['resources'][0][DOM::ATTRIBUTES]['base'] = $base_url;
  $wadl['resources'][0]['doc'][0][DOM::ATTRIBUTES]['title'] = "Doc stuff title info";
  $wadl['resources'][0]['doc'][0][DOM::CONTENT] = "Doc stuff here!!!";
  // TODO: functionize the creation of a resource?
  // Things needed: path
  $wadl['resources'][0]['resource'][] = array(
    DOM::ATTRIBUTES => array('path' => "path1 here!!"),
  );
  $wadl['resources'][0]['resource'][] = array(
    DOM::ATTRIBUTES => array('path' => "path2 here!!"),
  );
  
  $wadl['output']['now'] = print_r($wadl, TRUE);
  return $wadl;
}
