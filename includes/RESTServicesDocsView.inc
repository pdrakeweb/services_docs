<?php

/**
 * @file
 * Provide formatter class to handle WADL specific XML returns
 *
 */

/**
 * Class for overriding WADL specific XML returns from REST Server
 */
class RESTServicesDocsView extends RESTServerViewBuiltIn {
  public function render() {
    switch ($this->arguments['format']) {
      case 'xml':
        return $this->render_xml($this->model);
    }
    return parent::render();
  }

  private function render_xml($data) {
    module_load_include('php', 'services_docs', 'lib/DOM-2.0');
    // return DOM::arrayToXMLString($data, "application");
    $dom_data = DOM::arrayToDOMDocument($data, "application");
    // TODO: Somehow write application tag attributes - SimpleXML? DOMDocument?
    return $dom_data->saveXML();
  }

}
