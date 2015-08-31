{% comment %}Output values are assigned to variables named in CAPITALS.{% endcomment %}
{% assign RELATIVE_PATH = include.input_path | remove: '/Specification' %}
{% assign filename = RELATIVE_PATH | split: '/' | last %}
{% capture filename_with_slash %}/{{filename}}{% endcapture %}
{% assign NAMESPACE = RELATIVE_PATH | remove: '/dsdl/' | remove: filename_with_slash | replace: '/', '.' %}
{% assign tmp1 = filename | remove: '.uavcan' %}
{% if tmp1 contains '.' %}
    {% assign DEFAULT_DTID = tmp1 | split: '.' | first %}
    {% assign TYPE_NAME = tmp1 | split: '.' | last %}
{% else %}
    {% assign DEFAULT_DTID = null %}
    {% assign TYPE_NAME = tmp1 %}
{% endif %}
{% capture FULL_TYPE_NAME %}{{NAMESPACE}}.{{TYPE_NAME}}{% endcapture %}
