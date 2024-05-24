module {
  %plio_in = adf.plio.create() : !adf.plio<In, 128>
  %gmio_in = adf.gmio.create() : !adf.gmio<In, 128>
  %gmio_out0,%gmio_out1 = adf.graph(%plio_in,%gmio_in){

  } : (!adf.plio<In, 128>, !adf.gmio<In, 128>) -> (!adf.gmio<Out, 128>,!adf.gmio<Out, 128>)
}

