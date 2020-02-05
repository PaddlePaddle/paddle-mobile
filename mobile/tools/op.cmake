set(FOUND_MATCH OFF)
set(CON -1)

message(STATUS "nets :${NET}")

list(FIND NET "googlenet" CON)
if (CON GREATER -1)
  message("googlenet enabled")
  set(CONCAT_OP ON)
  set(CONV_OP ON)
  set(LRN_OP ON)
  set(MUL_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(FUSION_FC_OP ON)
  set(POOL_OP ON)
  set(RELU_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(FUSION_CONVADDRELU_OP ON)

  set(FOUND_MATCH ON)
endif()

list(FIND NET "mobilenet" CON)
if (CON GREATER -1)
  message("mobilenet enabled")
  set(CONV_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(RELU_OP ON)
  set(SOFTMAX_OP ON)
  set(MUL_OP ON)
  set(DEPTHWISECONV_OP ON)
  set(BATCHNORM_OP ON)
  set(POOL_OP ON)
  set(RESHAPE_OP ON)
  set(FUSION_CONVADDBNRELU_OP ON)
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_CONVADD_OP ON)

  set(FOUND_MATCH ON)
endif()


list(FIND NET "mobilenetssd" CON)
if (CON GREATER -1)
  message("mobilenetssd enabled")
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_DWCONVBNRELU_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(MULTICLASSNMS_OP ON)
  set(SOFTMAX_OP ON)
  set(TRANSPOSE_OP ON)
    #feed
  set(PRIORBOX_OP ON)
  set(CONCAT_OP ON)
  set(BOXCODER_OP ON)
  set(RESHAPE_OP ON)
#fetch
  #total

  set(FOUND_MATCH ON)

endif()


list(FIND NET "yolo" CON)
if (CON GREATER -1)
  message("yolo enabled")
  set(BATCHNORM_OP ON)
  set(CONV_OP ON)
  set(RELU_OP ON)
  set(ELEMENTWISEADD_OP ON)

  set(FOUND_MATCH ON)
endif()

list(FIND NET "squeezenet" CON)
if (CON GREATER -1)
  message("squeezenet enabled")
  set(CONCAT_OP ON)
  set(CONV_OP ON)
  set(RELU_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(POOL_OP ON)
  set(RESHAPE_OP ON)
  set(SOFTMAX_OP ON)

  set(FOUND_MATCH ON)
endif()


list(FIND NET "resnet" CON)
if (CON GREATER -1)
  message("resnet enabled")
  set(CONCAT_OP ON)
  set(CONV_OP ON)
  set(RELU_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(POOL_OP ON)
  set(BATCHNORM_OP ON)
  set(FUSION_CONVBNADDRELU_OP ON)
  set(MUL_OP ON)
  set(RESHAPE_OP ON)
  set(SOFTMAX_OP ON)
  set(FOUND_MATCH ON)
endif()

list(FIND NET "FPGA_NET_V1" CON)
if (CON GREATER -1)
  message("FPGA_NET_V1 enabled")
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_ELEMENTWISEADDRELU_OP ON)
  set(FUSION_FC_OP ON)
  set(POOL_OP ON)
  set(SOFTMAX_OP ON)
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_CONVBN_OP ON)
  set(TANH_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(TRANSPOSE2_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(SPLIT_OP ON)
  set(FUSION_DECONVADD_OP ON)
  set(FUSION_DECONVADDRELU_OP ON)

  set(RESHAPE_OP ON)
  set(FUSION_CONVADDBNRELU_OP ON)
  set(FUSION_CONVADDBN_OP ON)
  set(RESHAPE2_OP ON)
  set(PSROI_POOL_OP ON)
  set(ROIALIGN_POOL_OP ON)
  set(PROPOSAL_OP ON)
  set(ANCHOR_GENERATOR_OP ON)
  set(SLICE_OP ON)
  set(SIGMOID_OP ON)
  set(CONCAT_OP ON)
  set(PAD2D_OP ON)
  set(CONV_TRANSPOSE_OP ON)
  set(FUSION_DECONVADDBNRELU_OP ON)
  set(FUSION_DECONVADDBN_OP ON)
  set(FUSION_DECONVBNRELU_OP ON)
  set(CONV_OP ON)
  set(ELEMENTWISEMUL_OP ON)
  set(FUSION_FCRELU_OP ON)
  set(RELU_OP ON)
  set(FOUND_MATCH ON)
endif()

list(FIND NET "FPGA_NET_V2" CON)
if (CON GREATER -1)
  message("FPGA_NET_V2 enabled")
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_ELEMENTWISEADDRELU_OP ON)
  set(FUSION_FC_OP ON)
  set(POOL_OP ON)
  set(SOFTMAX_OP ON)
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_CONVBN_OP ON)
  set(TANH_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(TRANSPOSE2_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(SPLIT_OP ON)
  set(FUSION_DECONVADD_OP ON)
  set(FUSION_DECONVADDRELU_OP ON)

  set(RESHAPE_OP ON)
  set(FUSION_CONVADDBNRELU_OP ON)
  set(FUSION_CONVADDBN_OP ON)
  set(RESHAPE2_OP ON)
  set(PSROI_POOL_OP ON)
  set(ROIALIGN_POOL_OP ON)
  set(PROPOSAL_OP ON)
  set(ANCHOR_GENERATOR_OP ON)
  set(SLICE_OP ON)
  set(SIGMOID_OP ON)
  set(CONCAT_OP ON)
  set(CONV_TRANSPOSE_OP ON)
  set(FUSION_DECONVADDBNRELU_OP ON)
  set(FUSION_DECONVADDBN_OP ON)
  set(FUSION_DECONVBNRELU_OP ON)
  set(CONV_OP ON)
  set(ELEMENTWISEMUL_OP ON)
  set(FUSION_FCRELU_OP ON)
  set(RELU_OP ON)
  set(FOUND_MATCH ON)
endif()

list(FIND NET "FPGA_OPS_KD" CON)
if (CON GREATER -1)
  message("FPGA_OPS_KD enabled")
  set(CONV_OP ON)
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_ELEMENTWISEADDRELU_OP ON)
  set(FUSION_FC_OP ON)
  set(POOL_OP ON)
  set(SOFTMAX_OP ON)
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_CONVBN_OP ON)
  set(TANH_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(TRANSPOSE2_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(SPLIT_OP ON)
  set(FUSION_DECONVADD_OP ON)
  set(FUSION_DECONVADDRELU_OP ON)
  set(FOUND_MATCH ON)
endif()

list(FIND NET "nlp" CON)
if (CON GREATER -1)
  message("nlp enabled")
  set(FUSION_FC_OP ON)
  set(LOOKUP_OP ON)
  set(GRU_OP ON)
  set(CRF_OP ON)
  set(CONCAT_OP ON)
  set(ELEMENTWISEADD_OP ON)


  set(FOUND_MATCH ON)
endif()

list(FIND NET "mobilenetfssd" CON)
if (CON GREATER -1)
  message("mobilenetfssd enabled")
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_CONVADDBNRELU_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(SOFTMAX_OP ON)
  set(RESHAPE_OP ON)
  set(BILINEAR_INTERP_OP ON)
  set(TRANSPOSE_OP ON)
  set(CONCAT_OP ON)
  set(PRIORBOX_OP ON)
  set(BATCHNORM_OP ON)
  set(BOXCODER_OP ON)
  set(MULTICLASSNMS_OP ON)
  set(FLATTEN_OP ON)
  set(FLATTEN2_OP ON)
  set(SPLIT_OP ON)
  set(SHAPE_OP ON)

  set(FOUND_MATCH ON)
endif()

list(FIND NET "genet" CON)
if (CON GREATER -1)
  message("genet enabled")
  set(FUSION_CONVADDPRELU_OP ON)
  set(FUSION_CONVADDADDPRELU_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(CONV_TRANSPOSE_OP ON)
  set(FUSION_CONVADDRELU_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(PRELU_OP ON)
  set(POOL_OP ON)
  set(CONCAT_OP ON)

  set(FOUND_MATCH ON)
endif()

list(FIND NET "super" CON)
if (CON GREATER -1)
  message("super enabled")
  set(FUSION_CONVADD_OP ON)
  set(FUSION_CONVADDRELU_OP ON)
  set(ELEMENTWISEADD_OP ON)

  set(FOUND_MATCH ON)
endif()

list(FIND NET "op" CON)
if (CON GREATER -1)
  message("op enabled")
#  set(SIGMOID_OP ON)
#  set(LEAKY_RELU_OP ON)
  set(BLOG ON)
  set(FOUND_MATCH ON)
endif()

if(NOT FOUND_MATCH)
  message("--default--")
  set(NORM_OP ON)
  set(BATCHNORM_OP ON)
  set(INSTANCENORM_OP ON)
  set(CONV_TRANSPOSE_OP ON)
  set(BOXCODER_OP ON)
  set(CONCAT_OP ON)
  set(CONV_OP ON)
  set(DEPTHWISECONV_OP ON)
  set(ELEMENTWISEADD_OP ON)
  set(ELEMENTWISESUB_OP ON)
  set(IM2SEQUENCE_OP ON)
  set(FILL_CONSTANT_OP ON)
  set(DENSITY_PRIORBOX_OP ON)
  set(FUSION_CONVADD_OP ON)
  set(FUSION_CONVADDPRELU_OP ON)
  set(EXP_OP ON)
  set(FUSION_CONVADDRELU_OP ON)
  set(FUSION_FC_OP ON)
  set(LRN_OP ON)
  set(MUL_OP ON)
  set(MULTICLASSNMS_OP ON)
  set(POLYGONBOXTRANSFORM_OP ON)
  set(POOL_OP ON)
  set(PRIORBOX_OP ON)
  set(RELU_OP ON)
  set(RESHAPE_OP ON)
  set(RESHAPE2_OP ON)
  set(SIGMOID_OP ON)
  set(SOFTMAX_OP ON)
  set(TRANSPOSE_OP ON)
  set(TRANSPOSE2_OP ON)
  set(FUSION_CONVADDBNRELU_OP ON)
  set(FUSION_CONVADDADDPRELU_OP ON)
  set(FUSION_DWCONVBNRELU_OP ON)
  set(FUSION_CONVBNRELU_OP ON)
  set(FUSION_CONVRELU_OP ON)
  set(FUSION_CONVBNADDRELU_OP ON)
  set(PRELU_OP ON)
  set(RESIZE_OP ON)
  set(SCALE_OP ON)
  set(SLICE_OP ON)
  set(DROPOUT_OP ON)
  set(IM2SEQUENCE_OP ON)
  set(LOOKUP_OP ON)
  set(GRU_OP ON)
  set(GRU_UNIT_OP ON)
  set(CRF_OP ON)
  set(BILINEAR_INTERP_OP ON)
  set(SPLIT_OP ON)
  set(FLATTEN_OP ON)
  set(FLATTEN2_OP ON)
  set(SHAPE_OP ON)
  set(ELEMENTWISEMUL_OP ON)
  set(SUM_OP ON)
  set(TOP_K_OP ON)
  set(CAST_OP ON)
  set(QUANT_OP ON)
  set(DEQUANT_OP ON)
  set(FUSION_DEQUANT_BN_OP ON)
  set(FUSION_DEQUANT_ADD_BN_OP ON)
  set(FUSION_DEQUANT_BN_RELU_OP ON)
  set(FUSION_DEQUANT_ADD_BN_RELU_OP ON)
  set(FUSION_DEQUANT_ADD_BN_QUANT_OP ON)
  set(FUSION_DEQUANT_ADD_BN_RELU_QUANT_OP ON)
  set(SEQUENCE_EXPAND_OP ON)
  set(SEQUENCE_POOL_OP ON)
  set(SEQUENCE_SOFTMAX_OP ON)
  set(LOG_OP ON)
  set(TANH_OP ON)
  set(LOD_RESET_OP ON)
  set(LESS_THAN_OP ON)
  set(LOGICAL_AND_OP ON)
  set(LOGICAL_OR_OP ON)
  set(LOGICAL_NOT_OP ON)
  set(LOGICAL_XOR_OP ON)
  set(WHILE_OP ON)
  set(WRITE_TO_ARRAY_OP ON)
  set(READ_FROM_ARRAY_OP ON)
  set(IS_EMPTY_OP ON)
  set(INCREMENT_OP ON)
  set(ANCHOR_GENERATOR_OP ON)
  set(PROPOSAL_OP ON)
  set(PSROI_POOL_OP ON)
  set(ROI_PERSPECTIVE_OP ON)
  set(BEAM_SEARCH_OP ON)
  set(BEAM_SEARCH_DECODE_OP ON)
  set(PAD2D_OP ON)
  set(ONE_HOT_OP ON)
  set(ASSIGN_VALUE_OP ON)
  set(NEAREST_INTERP_OP ON)
  set(LEAKY_RELU_OP ON)
  set(ASSIGN_OP ON)
  set(CONDITIONAL_BLOCK_OP ON)
  set(EQUAL_OP ON)
  set(FILL_CONSTANT_BATCH_SIZE_LIKE_OP ON)
  set(RANGE_OP ON)
  set(REDUCE_PROD_OP ON)
  set(FUSION_INSTANCENORM_RELU_OP ON)
  set(PIXEL_SHUFFLE_OP ON)
  set(EXPAND_OP ON)
  set(GRID_SAMPLER_OP ON)
endif()

  # option(BATCHNORM_OP "" ON)
  # option(BOXCODER_OP "" ON)
  # option(CONCAT_OP "" ON)
  # option(CONV_OP "" ON)
  # option(DEPTHWISECONV_OP "" ON)
  # option(ELEMENTWISEADD_OP "" ON)
  # option(FILL_CONSTANT_OP "" ON)
  # option(FUSION_CONVADD_OP "" ON)
  # option(FUSION_CONVADDRELU_OP "" ON)
  # option(FUSION_FC_OP "" ON)
  # option(LRN_OP "" ON)
  # option(MUL_OP "" ON)
  # option(MULTICLASSNMS_OP "" ON)
  # option(POLYGONBOXTRANSFORM_OP "" ON)
  # option(POOL_OP "" ON)
  # option(PRIORBOX_OP "" ON)
  # option(RELU_OP "" ON)
  # option(RESHAPE_OP "" ON)
  # option(RESHAPE2_OP "" ON)
  # option(SIGMOID_OP "" ON)
  # option(SOFTMAX_OP "" ON)
  # option(TRANSPOSE_OP "" ON)
  # option(TRANSPOSE2_OP "" ON)
# endif ()

if (NORM_OP)
  add_definitions(-DNORM_OP)
endif()
if (BATCHNORM_OP)
  add_definitions(-DBATCHNORM_OP)
endif()
if (INSTANCENORM_OP)
  add_definitions(-DINSTANCENORM_OP)
endif()
if (FUSION_INSTANCENORM_RELU_OP)
  add_definitions(-DFUSION_INSTANCENORM_RELU_OP)
endif()
if (BOXCODER_OP)
  add_definitions(-DBOXCODER_OP)
endif()
if (CONCAT_OP)
  add_definitions(-DCONCAT_OP)
endif()
if (CONV_OP)
  add_definitions(-DCONV_OP)
endif()
if (DEPTHWISECONV_OP)
  add_definitions(-DDEPTHWISECONV_OP)
endif()
if (ELEMENTWISEADD_OP)
  add_definitions(-DELEMENTWISEADD_OP)
endif()
if (ELEMENTWISESUB_OP)
  add_definitions(-DELEMENTWISESUB_OP)
endif()
if (FILL_CONSTANT_OP)
  add_definitions(-DFILL_CONSTANT_OP)
endif()
if (FUSION_CONVADD_OP)
  add_definitions(-DFUSION_CONVADD_OP)
endif()
if (FUSION_CONVADDRELU_OP)
  add_definitions(-DFUSION_CONVADDRELU_OP)
endif()
if (FUSION_CONVADDPRELU_OP)
  add_definitions(-DFUSION_CONVADDPRELU_OP)
endif()
if (FUSION_CONVADDADDPRELU_OP)
  add_definitions(-DFUSION_CONVADDADDPRELU_OP)
endif()
if (FUSION_FC_OP)
  add_definitions(-DFUSION_FC_OP)
endif()
if (LRN_OP)
  add_definitions(-DLRN_OP)
endif()
if (MUL_OP)
  add_definitions(-DMUL_OP)
endif()
if (MULTICLASSNMS_OP)
  add_definitions(-DMULTICLASSNMS_OP)
endif()
if (POLYGONBOXTRANSFORM_OP)
  add_definitions(-DPOLYGONBOXTRANSFORM_OP)
endif()
if (POOL_OP)
  add_definitions(-DPOOL_OP)
endif()
if (PRIORBOX_OP)
  add_definitions(-DPRIORBOX_OP)
endif()
if (RELU_OP)
  add_definitions(-DRELU_OP)
endif()
if (RESHAPE_OP)
  add_definitions(-DRESHAPE_OP)
endif()
if (RESHAPE2_OP)
  add_definitions(-DRESHAPE2_OP)
endif()
if (SIGMOID_OP)
  add_definitions(-DSIGMOID_OP)
endif()
if (SOFTMAX_OP)
  add_definitions(-DSOFTMAX_OP)
endif()
if (TRANSPOSE_OP)
  add_definitions(-DTRANSPOSE_OP)
endif()
if (TRANSPOSE2_OP)
  add_definitions(-DTRANSPOSE2_OP)
endif()
if (FUSION_CONVADDBNRELU_OP)
  add_definitions(-DFUSION_CONVADDBNRELU_OP)
endif()
if (FUSION_DWCONVBNRELU_OP)
  add_definitions(-DFUSION_DWCONVBNRELU_OP)
endif()

if (FUSION_CONVBNRELU_OP)
  add_definitions(-DFUSION_CONVBNRELU_OP)
endif()

if (FUSION_CONVRELU_OP)
  add_definitions(-DFUSION_CONVRELU_OP)
endif()

if (FUSION_CONVBNADDRELU_OP)
  add_definitions(-DFUSION_CONVBNADDRELU_OP)
endif()

if (PRELU_OP)
  add_definitions(-DPRELU_OP)
endif()
if (RESIZE_OP)
  add_definitions(-DRESIZE_OP)
endif()
if (SCALE_OP)
  add_definitions(-DSCALE_OP)
endif()
if (SLICE_OP)
  add_definitions(-DSLICE_OP)
endif()
if (DROPOUT_OP)
  add_definitions(-DDROPOUT_OP)
endif()
if (IM2SEQUENCE_OP)
  add_definitions(-DIM2SEQUENCE_OP)
endif()

if (FUSION_CONVADDBN_OP)
  add_definitions(-DFUSION_CONVADDBN_OP)
endif()
if (FUSION_FCRELU_OP)
  add_definitions(-DFUSION_FCRELU_OP)
endif()
if (FUSION_POOLBN_OP)
  add_definitions(-DFUSION_POOLBN_OP)
endif()
if (FUSION_ELEMENTWISEADDRELU_OP)
  add_definitions(-DFUSION_ELEMENTWISEADDRELU_OP)
endif()
if (FUSION_CONVBN_OP)
  add_definitions(-DFUSION_CONVBN_OP)
endif()

if (CONV_TRANSPOSE_OP)
  add_definitions(-DCONV_TRANSPOSE_OP)
endif()

if (LOOKUP_OP)
  add_definitions(-DLOOKUP_OP)
endif()

if (GRU_OP)
  add_definitions(-DGRU_OP)
endif()

if (GRU_UNIT_OP)
  add_definitions(-DGRU_UNIT_OP)
endif()

if (CRF_OP)
  add_definitions(-DCRF_OP)
endif()


if (FLATTEN_OP)
  add_definitions(-DFLATTEN_OP)
endif()

if (FLATTEN2_OP)
  add_definitions(-DFLATTEN2_OP)
endif()

if (SPLIT_OP)
  add_definitions(-DSPLIT_OP)
endif()

if (BILINEAR_INTERP_OP)
  add_definitions(-DBILINEAR_INTERP_OP)
endif()

if (SHAPE_OP)
  add_definitions(-DSHAPE_OP)
endif()

if (ELEMENTWISEMUL_OP)
  add_definitions(-DELEMENTWISEMUL_OP)
endif()
if (SUM_OP)
  add_definitions(-DSUM_OP)
endif()
if (TOP_K_OP)
  add_definitions(-DTOP_K_OP)
endif()
if (CAST_OP)
  add_definitions(-DCAST_OP)
endif()
if (QUANT_OP)
  add_definitions(-DQUANT_OP)
endif()
if (DEQUANT_OP)
  add_definitions(-DDEQUANT_OP)
endif()
if (FUSION_DEQUANT_BN_OP)
  add_definitions(-DFUSION_DEQUANT_BN_OP)
endif()
if (FUSION_DEQUANT_ADD_BN_OP)
  add_definitions(-DFUSION_DEQUANT_ADD_BN_OP)
endif()
if (FUSION_DEQUANT_BN_RELU_OP)
  add_definitions(-DFUSION_DEQUANT_BN_RELU_OP)
endif()
if (FUSION_DEQUANT_ADD_BN_RELU_OP)
  add_definitions(-DFUSION_DEQUANT_ADD_BN_RELU_OP)
endif()
if (FUSION_DEQUANT_ADD_BN_QUANT_OP)
#  add_definitions(-DFUSION_DEQUANT_ADD_BN_QUANT_OP)
endif()
if (FUSION_DEQUANT_ADD_BN_RELU_QUANT_OP)
#  add_definitions(-DFUSION_DEQUANT_ADD_BN_RELU_QUANT_OP)
endif()
if (SEQUENCE_EXPAND_OP)
  add_definitions(-DSEQUENCE_EXPAND_OP)
endif()
if (SEQUENCE_POOL_OP)
  add_definitions(-DSEQUENCE_POOL_OP)
endif()
if (SEQUENCE_SOFTMAX_OP)
  add_definitions(-DSEQUENCE_SOFTMAX_OP)
endif()
if (LOG_OP)
  add_definitions(-DLOG_OP)
endif()
if (LOD_RESET_OP)
  add_definitions(-DLOD_RESET_OP)
endif()
if (LESS_THAN_OP)
  add_definitions(-DLESS_THAN_OP)
endif()
if (LOGICAL_AND_OP)
  add_definitions(-DLOGICAL_AND_OP)
endif()
if (LOGICAL_OR_OP)
  add_definitions(-DLOGICAL_OR_OP)
endif()
if (LOGICAL_NOT_OP)
  add_definitions(-DLOGICAL_NOT_OP)
endif()
if (LOGICAL_XOR_OP)
  add_definitions(-DLOGICAL_XOR_OP)
endif()

if (TANH_OP)
  add_definitions(-DTANH_OP)
endif()
if (FUSION_DECONVRELU_OP)
  add_definitions(-DFUSION_DECONVRELU_OP)
endif()
if (FUSION_DECONVADD_OP)
  add_definitions(-DFUSION_DECONVADD_OP)
endif()
if (FUSION_DECONVADDRELU_OP)
  add_definitions(-DFUSION_DECONVADDRELU_OP)
endif()
if (WHILE_OP)
  add_definitions(-DWHILE_OP)
endif()
if (WRITE_TO_ARRAY_OP)
  add_definitions(-DWRITE_TO_ARRAY_OP)
endif()
if (READ_FROM_ARRAY_OP)
  add_definitions(-DREAD_FROM_ARRAY_OP)
endif()
if (IS_EMPTY_OP)
  add_definitions(-DIS_EMPTY_OP)
endif()
if (INCREMENT_OP)
  add_definitions(-DINCREMENT_OP)
endif()

if (ANCHOR_GENERATOR_OP)
  add_definitions(-DANCHOR_GENERATOR_OP)
endif()
if (PROPOSAL_OP)
  add_definitions(-DPROPOSAL_OP)
endif()
if (PSROI_POOL_OP)
  add_definitions(-DPSROI_POOL_OP)
endif()
if (ROIALIGN_POOL_OP)
  add_definitions(-DROIALIGN_POOL_OP)
endif()
if (ROI_PERSPECTIVE_OP)
  add_definitions(-DROI_PERSPECTIVE_OP)
endif()
if (BEAM_SEARCH_OP)
  add_definitions(-DBEAM_SEARCH_OP)
endif()
if (BEAM_SEARCH_DECODE_OP)
  add_definitions(-DBEAM_SEARCH_DECODE_OP)
endif()
if (FUSION_DECONVADDBNRELU_OP)
  add_definitions(-DFUSION_DECONVADDBNRELU_OP)
endif()
if (FUSION_DECONVBNRELU_OP)
  add_definitions(-DFUSION_DECONVBNRELU_OP)
endif()
if (FUSION_DECONVADDBN_OP)
  add_definitions(-DFUSION_DECONVADDBN_OP)
endif()
if (PAD2D_OP)
  add_definitions(-DPAD2D_OP)
endif()
if (ONE_HOT_OP)
  add_definitions(-DONE_HOT_OP)
endif()
if (ASSIGN_VALUE_OP)
  add_definitions(-DASSIGN_VALUE_OP)
endif()
if (LEAKY_RELU_OP)
  add_definitions(-DLEAKY_RELU_OP)
endif()
if (NEAREST_INTERP_OP)
  add_definitions(-DNEAREST_INTERP_OP)
endif()
if (DENSITY_PRIORBOX_OP)
  add_definitions(-DDENSITY_PRIORBOX_OP)
endif()
if (EXP_OP)
  add_definitions(-DEXP_OP)
endif ()
if (ASSIGN_OP)
  add_definitions(-DASSIGN_OP)
endif()
if (CONDITIONAL_BLOCK_OP)
  add_definitions(-DCONDITIONAL_BLOCK_OP)
endif()
if (EQUAL_OP)
  add_definitions(-DEQUAL_OP)
endif()
if (FILL_CONSTANT_BATCH_SIZE_LIKE_OP)
  add_definitions(-DFILL_CONSTANT_BATCH_SIZE_LIKE_OP)
endif()
if (RANGE_OP)
  add_definitions(-DRANGE_OP)
endif()
if (REDUCE_PROD_OP)
  add_definitions(-DREDUCE_PROD_OP)
endif()
if (PIXEL_SHUFFLE_OP)
  add_definitions(-DPIXEL_SHUFFLE_OP)
endif()
if (EXPAND_OP)
  add_definitions(-DEXPAND_OP)
endif()
if (GRID_SAMPLER_OP)
  add_definitions(-DGRID_SAMPLER_OP)
endif()
if (BLOG)
  add_definitions(-DBLOG)
endif()

