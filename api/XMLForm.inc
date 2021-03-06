<?php

/**
 * @file
 * Implementation of the objective_forms Form class.
 */

module_load_include('inc', 'objective_forms', 'Form');
module_load_include('inc', 'xml_form_api', 'XMLDocument');
module_load_include('inc', 'xml_form_api', 'Actions');

/**
 * The XML Form class.
 */
class XMLForm extends Form {

  /**
   * The document the form manipulates.
   *
   * @var XMLDocument
   */
  protected $document;

  /**
   * Constructor function for the XMLForm class.
   *
   * @param array $form_state
   *   The form state.
   */
  public function __construct(array &$form_state) {
    parent::__construct(array(), $form_state);
    if ($this->storage->initialized) {
      $this->initializeFromStorage();
    }
  }

  /**
   * Initialize this object from values in storage.
   */
  protected function initializeFromStorage() {
    $this->root = $this->storage->root;
    $this->document = $this->storage->document;
  }

  /**
   * Stores information required to rebuild this object.
   */
  protected function storePersistantMembers() {
    $this->storage->root = $this->root;
    $this->storage->document = $this->document;
    $this->storage->initialized = TRUE;
  }

  /**
   * Initializes this object's members.
   *
   * This function should be called the first time this object is created for a
   * particular form; on subsequent submit/validation/etc callbacks,
   * Form::initializeFromStorage() will be called.
   *
   * @throws Exception
   *   If an attempt is made to initialize a form a second time.
   *
   * @param array $form
   *   The form to initialize.
   * @param XMLDocument $document
   *   The XML document to initialize.
   */
  public function initialize(array &$form, XMLDocument $document) {
    if (!$this->initialized) {
      $this->root = new FormElement($this->registry, $form);
      $this->document = $document;
      // Register nodes and generate new elements.
      $this->generate();
      $this->prepopulate();
      $this->storePersistantMembers();
    }
    else {
      throw new Exception('Attempted to intialize the form after it has already been intialized.');
    }
  }

  /**
   * Determines if the form has been initialized.
   *
   * @return bool
   *   TRUE or FALSE depending on whether or not the form is initialized.
   */
  public function isInitialized() {
    return $this->storage->initialized;
  }

  /**
   * Generates additional elements based on the XMLDocument's nodes.
   *
   * Called the first time this form is created.
   *
   * @return array
   *   Array of elements generated.
   */
  protected function generate() {
    module_load_include('inc', 'xml_form_api', 'XMLFormGenerator');
    $generator = new XMLFormGenerator($this, $this->document);
    $generator->generate($this->root);
  }

  /**
   * Clones the existing set of elements.
   *
   * These elements will not be saved in the storage.
   */
  public function prepopulate() {
    module_load_include('inc', 'xml_form_api', 'XMLFormPrePopulator');
    $populator = new XMLFormPrePopulator($this->document);
    $populator->prepopulate($this->root);
  }

  /**
   * Submits the form.
   *
   * @param array $form
   *   The form to submit.
   * @param array $form_state
   *   The state of the form.
   *
   * @return XMLDocument
   *   An XML document generated from the form.
   */
  public function submit(array &$form, array &$form_state) {
    module_load_include('inc', 'objective_forms', 'FormValues');
    module_load_include('inc', 'xml_form_api', 'XMLFormProcessor');

    $flat = $this->flattenDrupalForm($form);
    $form_element = $this->registry->get($form['#hash']);
    $descendant_access_change = function ($descendant, $flat_array) {
      if (isset($flat_array[$descendant->hash]) && $flat_array[$descendant->hash]['#access'] != $descendant['#access']) {
        $descendant['#access'] = $flat_array[$descendant->hash]['#access'];
      }
    };
    $form_element->eachDecendant($descendant_access_change, $flat);
    $form_values = new FormValues($form_state, $form, $this->registry);
    $processor = new XMLFormProcessor($form_values, $this->document, $this->registry);
    return $processor->process($this->root);
  }

  /**
   * Flattens the Drupal form to be used when checking against the registry.
   *
   * @param array $form
   *   An array representing a form within Drupal.
   *
   * @return array
   *   An array where the keys are the hash of the FormElement and the value is
   *   the corresponding FormElement.
   */
  public function flattenDrupalForm($form) {
    $flattened = array();
    foreach (element_children($form) as $key) {
      $child_element = $form[$key];
      if (isset($child_element['#hash'])) {
        $flattened[$child_element['#hash']] = $child_element;
        $flattened = array_merge($flattened, $this->flattenDrupalForm($child_element));
      }
    }
    return $flattened;
  }
}
