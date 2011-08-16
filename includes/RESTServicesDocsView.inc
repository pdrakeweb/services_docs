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
    // TODO: Check if we are processing a WADL doc
    switch ($this->arguments['format']) {
      case 'xml':
        return $this->render_xml($this->model);
    }
    return parent::render();
  }

  private function render_xml($data) {
    // NOTES: Trying to handle structures PHP<->XML data properly
    // *  Pear::XML_Serializer - http://pear.php.net/package/XML_Serializer/
    // *  http://code.google.com/p/array-to-domdocument/
    // *  http://stackoverflow.com/questions/2247608/is-it-wrong-to-transform-unserialize-xml-to-an-array-to-interface-with-it
    // *  http://stackoverflow.com/questions/1397036/how-to-convert-array-to-simplexml-in-php

    module_load_include('inc', 'services_docs', 'includes/wadl');
    module_load_include('php', 'services_docs', 'lib/DOM-2.0');
    $dom_data = DOM::arrayToDOMDocument($data, WADL::APPLICATION);

    // processing instruction data
    $styleheetParams = 'href="/wadl.xsl" type="text/xsl"';
    $xmlstylesheet = new DOMProcessingInstruction( 'xml-stylesheet', $styleheetParams);

    //prepend it to the doc
    $dom_data->insertBefore($xmlstylesheet, $dom_data->firstChild);
    $dom_data->normalize();

    // TODO: Somehow write application tag attributes - SimpleXML? DOMDocument?
    return $dom_data->saveXML();
  }

}
