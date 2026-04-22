build_lib_example(
  NAME composite-energy-model-example
  SOURCE_FILES composite-energy-model-example.cc
  LIBRARIES_TO_LINK
    ${libcomposite-energy}
    ${libcore}
    ${libnetwork}
    ${libenergy}
    ${libmobility}
    ${libwifi}
)
