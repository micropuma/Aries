
//===----------------------------------------------------------------------===//
//
// Automatically generated file for hlc.cpp
//
//===----------------------------------------------------------------------===//
#include <math.h>
#include <stdint.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_math.h>
void send3_0(
  hls::stream< ap_int<128> > &v40 /* v40[1] */,
  ap_int<128> v41[32][8],
  bool v42
){
  #pragma HLS inline OFF
  if (v42) {	// L150
    for (int v43 = 0; v43 < 1; v43++) {	// L151
      for (int v44 = 0; v44 < 32; v44++) {	// L152
        for (int v45 = 0; v45 < 1; v45++) {	// L153
          for (int v46 = 0; v46 < 8; v46++) {	// L154
          #pragma HLS pipeline II=1
            ap_int<128> v47 = v40.read(); //v40            v47 = v40;	// L155
            v41[(v44 + (v43 * 32))][(v46 + (v45 * 8))] = v47;	// L156
          }
        }
      }
    }
  }
}

void send3_1(
  ap_int<128> v48[32][8],
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v49 /* v49[1] */,
  bool v50
){
  #pragma HLS inline OFF
  if (v50) {	// L165
    for (int v51 = 0; v51 < 1; v51++) {	// L166
      for (int v52 = 0; v52 < 1; v52++) {	// L167
        for (int v53 = 0; v53 < 1; v53++) {	// L168
          for (int v54 = 0; v54 < 32; v54++) {	// L169
            for (int v55 = 0; v55 < 8; v55++) {	// L170
            #pragma HLS pipeline II=1
              ap_int<128> v56 = v48[(v54 + (v53 * 32))][(v55 + (v52 * 8))];	// L171
              ap_axiu<128, 0 ,0 ,0> v49_axiu;
              v49_axiu.data = v56;
              v49_axiu.keep = -1;
              v49.write(v49_axiu); //v49              v49 = v56;	// L172
            }
          }
        }
      }
    }
  }
}

template<int NC>
void send3(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v57 /* v57[1] */,
  hls::stream< ap_int<128> > &v58 /* v58[1] */
){
  #pragma HLS inline OFF
  ap_int<128> v59[32][8];	// L185
  #pragma HLS bind_storage variable=v59 type=ram_s2p impl=bram
  ap_int<128> v60[32][8];	// L186
  #pragma HLS bind_storage variable=v60 type=ram_s2p impl=bram
  for (int v61 = 0; v61 < 1; v61++) {	// L187
    for (int v62 = 0; v62 < 1; v62++) {	// L188
      for (int v63 = 0; v63 < 1; v63++) {	// L189
        int v64 = v63 + v62;	// L190
        int v65 = v64 + v61;	// L191
        int v66 = v65 % 2;	// L192
        bool v67 = v66 == 0;	// L193
        bool v68 = v65 != 0;	// L194
        if (v67) {	// L195
          send3_0(v58, v59, 1);	// L196
          send3_1(v60, v57, v68);	// L197
        } else {
          send3_0(v58, v60, 1);	// L199
          send3_1(v59, v57, v68);	// L200
        }
      }
    }
  }
  send3_1(v59, v57, 1);	// L205
}

void send3_top(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v69 /* v69[1] */,
  hls::stream< ap_int<128> > &v70 /* v70[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v71 /* v71[1] */,
  hls::stream< ap_int<128> > &v72 /* v72[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v73 /* v73[1] */,
  hls::stream< ap_int<128> > &v74 /* v74[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v75 /* v75[1] */,
  hls::stream< ap_int<128> > &v76 /* v76[1] */
){
  #pragma HLS inline OFF
  send3<0>(v69, v70);	// L209
  send3<1>(v71, v72);	// L210
  send3<2>(v73, v74);	// L211
  send3<3>(v75, v76);	// L212
}

template<int NC>
void receive2(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v77 /* v77[1] */,
  hls::stream< ap_int<128> > &v78 /* v78[1] */
){
  #pragma HLS inline OFF
  ap_int<128> v79[32][8];	// L225
  #pragma HLS bind_storage variable=v79 type=ram_t2p impl=uram
  for (int v80 = 0; v80 < 32; v80++) {	// L226
    for (int v81 = 0; v81 < 8; v81++) {	// L227
    #pragma HLS pipeline II=1
      v79[v80][v81] = 0;	// L228
    }
  }
  for (int v82 = 0; v82 < 1; v82++) {	// L231
    for (int v83 = 0; v83 < 1; v83++) {	// L232
      for (int v84 = 0; v84 < 1; v84++) {	// L233
        for (int v85 = 0; v85 < 1; v85++) {	// L234
          for (int v86 = 0; v86 < 1; v86++) {	// L235
            for (int v87 = 0; v87 < 1; v87++) {	// L236
              for (int v88 = 0; v88 < 32; v88++) {	// L237
                for (int v89 = 0; v89 < 8; v89++) {	// L238
                #pragma HLS pipeline II=1
                  ap_axiu<128, 0 ,0 ,0> v77_axiu = v77.read();
                  ap_int<128> v90 = v77_axiu.data; //v77                  v90 = v77;	// L239
                  ap_int<128> v91 = v79[(v88 + (v85 * 32))][(v89 + (v86 * 8))];	// L240
                  ap_int<128> v92 = v90;
                  ap_int<128> v93 = v91;
                  ap_int<128> v94 = 0;
                  int32_t v95 = v92(31, 0);	// L244
                  int32_t v96 = v93(31, 0);	// L245
                  float v97;
                  union { int32_t from; float to;} _converter_v95_to_v97;
                  _converter_v95_to_v97.from = v95;
                  v97 = _converter_v95_to_v97.to;	// L246
                  float v98;
                  union { int32_t from; float to;} _converter_v96_to_v98;
                  _converter_v96_to_v98.from = v96;
                  v98 = _converter_v96_to_v98.to;	// L247
                  float v99 = v97 + v98;	// L248
                  int32_t v100;
                  union { float from; int32_t to;} _converter_v99_to_v100;
                  _converter_v99_to_v100.from = v99;
                  v100 = _converter_v99_to_v100.to;	// L249
                  v94(31, 0) = v100;	// L250
                  int32_t v101 = v92(63, 32);	// L251
                  int32_t v102 = v93(63, 32);	// L252
                  float v103;
                  union { int32_t from; float to;} _converter_v101_to_v103;
                  _converter_v101_to_v103.from = v101;
                  v103 = _converter_v101_to_v103.to;	// L253
                  float v104;
                  union { int32_t from; float to;} _converter_v102_to_v104;
                  _converter_v102_to_v104.from = v102;
                  v104 = _converter_v102_to_v104.to;	// L254
                  float v105 = v103 + v104;	// L255
                  int32_t v106;
                  union { float from; int32_t to;} _converter_v105_to_v106;
                  _converter_v105_to_v106.from = v105;
                  v106 = _converter_v105_to_v106.to;	// L256
                  v94(63, 32) = v106;	// L257
                  int32_t v107 = v92(95, 64);	// L258
                  int32_t v108 = v93(95, 64);	// L259
                  float v109;
                  union { int32_t from; float to;} _converter_v107_to_v109;
                  _converter_v107_to_v109.from = v107;
                  v109 = _converter_v107_to_v109.to;	// L260
                  float v110;
                  union { int32_t from; float to;} _converter_v108_to_v110;
                  _converter_v108_to_v110.from = v108;
                  v110 = _converter_v108_to_v110.to;	// L261
                  float v111 = v109 + v110;	// L262
                  int32_t v112;
                  union { float from; int32_t to;} _converter_v111_to_v112;
                  _converter_v111_to_v112.from = v111;
                  v112 = _converter_v111_to_v112.to;	// L263
                  v94(95, 64) = v112;	// L264
                  int32_t v113 = v92(127, 96);	// L265
                  int32_t v114 = v93(127, 96);	// L266
                  float v115;
                  union { int32_t from; float to;} _converter_v113_to_v115;
                  _converter_v113_to_v115.from = v113;
                  v115 = _converter_v113_to_v115.to;	// L267
                  float v116;
                  union { int32_t from; float to;} _converter_v114_to_v116;
                  _converter_v114_to_v116.from = v114;
                  v116 = _converter_v114_to_v116.to;	// L268
                  float v117 = v115 + v116;	// L269
                  int32_t v118;
                  union { float from; int32_t to;} _converter_v117_to_v118;
                  _converter_v117_to_v118.from = v117;
                  v118 = _converter_v117_to_v118.to;	// L270
                  v94(127, 96) = v118;	// L271
                  ap_int<128> v119 = v94;
                  v79[(v88 + (v85 * 32))][(v89 + (v86 * 8))] = v119;	// L273
                }
              }
            }
          }
        }
      }
      for (int v120 = 0; v120 < 1; v120++) {	// L280
        for (int v121 = 0; v121 < 32; v121++) {	// L281
          for (int v122 = 0; v122 < 1; v122++) {	// L282
            for (int v123 = 0; v123 < 8; v123++) {	// L283
            #pragma HLS pipeline II=1
              ap_int<128> v124 = v79[(v121 + (v120 * 32))][(v123 + (v122 * 8))];	// L284
              v78.write(v124); //v78              v78 = v124;	// L285
              v79[(v121 + (v120 * 32))][(v123 + (v122 * 8))] = 0;	// L286
            }
          }
        }
      }
    }
  }
}

void receive2_top(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v125 /* v125[1] */,
  hls::stream< ap_int<128> > &v126 /* v126[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v127 /* v127[1] */,
  hls::stream< ap_int<128> > &v128 /* v128[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v129 /* v129[1] */,
  hls::stream< ap_int<128> > &v130 /* v130[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v131 /* v131[1] */,
  hls::stream< ap_int<128> > &v132 /* v132[1] */
){
  #pragma HLS inline OFF
  receive2<0>(v125, v126);	// L296
  receive2<1>(v127, v128);	// L297
  receive2<2>(v129, v130);	// L298
  receive2<3>(v131, v132);	// L299
}

void send6_0(
  hls::stream< ap_int<128> > &v133 /* v133[1] */,
  ap_int<128> v134[32][8],
  bool v135
){
  #pragma HLS inline OFF
  if (v135) {	// L303
    for (int v136 = 0; v136 < 1; v136++) {	// L304
      for (int v137 = 0; v137 < 32; v137++) {	// L305
        for (int v138 = 0; v138 < 1; v138++) {	// L306
          for (int v139 = 0; v139 < 8; v139++) {	// L307
          #pragma HLS pipeline II=1
            ap_int<128> v140 = v133.read(); //v133            v140 = v133;	// L308
            v134[(v137 + (v136 * 32))][(v139 + (v138 * 8))] = v140;	// L309
          }
        }
      }
    }
  }
}

void send6_1(
  ap_int<128> v141[32][8],
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v142 /* v142[1] */,
  bool v143
){
  #pragma HLS inline OFF
  if (v143) {	// L318
    for (int v144 = 0; v144 < 1; v144++) {	// L319
      for (int v145 = 0; v145 < 1; v145++) {	// L320
        for (int v146 = 0; v146 < 1; v146++) {	// L321
          for (int v147 = 0; v147 < 32; v147++) {	// L322
            for (int v148 = 0; v148 < 8; v148++) {	// L323
            #pragma HLS pipeline II=1
              ap_int<128> v149 = v141[(v147 + (v144 * 32))][(v148 + (v146 * 8))];	// L324
              ap_axiu<128, 0 ,0 ,0> v142_axiu;
              v142_axiu.data = v149;
              v142_axiu.keep = -1;
              v142.write(v142_axiu); //v142              v142 = v149;	// L325
            }
          }
        }
      }
    }
  }
}

template<int NC>
void send6(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v150 /* v150[1] */,
  hls::stream< ap_int<128> > &v151 /* v151[1] */
){
  #pragma HLS inline OFF
  ap_int<128> v152[32][8];	// L338
  #pragma HLS bind_storage variable=v152 type=ram_s2p impl=bram
  ap_int<128> v153[32][8];	// L339
  #pragma HLS bind_storage variable=v153 type=ram_s2p impl=bram
  for (int v154 = 0; v154 < 1; v154++) {	// L340
    for (int v155 = 0; v155 < 1; v155++) {	// L341
      for (int v156 = 0; v156 < 1; v156++) {	// L342
        int v157 = v156 + v155;	// L343
        int v158 = v157 + v154;	// L344
        int v159 = v158 % 2;	// L345
        bool v160 = v159 == 0;	// L346
        bool v161 = v158 != 0;	// L347
        if (v160) {	// L348
          send6_0(v151, v152, 1);	// L349
          send6_1(v153, v150, v161);	// L350
        } else {
          send6_0(v151, v153, 1);	// L352
          send6_1(v152, v150, v161);	// L353
        }
      }
    }
  }
  send6_1(v152, v150, 1);	// L358
}

void send6_top(
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v162 /* v162[1] */,
  hls::stream< ap_int<128> > &v163 /* v163[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v164 /* v164[1] */,
  hls::stream< ap_int<128> > &v165 /* v165[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v166 /* v166[1] */,
  hls::stream< ap_int<128> > &v167 /* v167[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v168 /* v168[1] */,
  hls::stream< ap_int<128> > &v169 /* v169[1] */
){
  #pragma HLS inline OFF
  send6<0>(v162, v163);	// L362
  send6<1>(v164, v165);	// L363
  send6<2>(v166, v167);	// L364
  send6<3>(v168, v169);	// L365
}

template<int NC>
void store0_0(
  hls::stream< ap_int<128> > &v170 /* v170[1] */,
  hls::stream< ap_int<512> > &v171 /* v171[1] */
){
  #pragma HLS inline OFF
  for (int v172 = 0; v172 < 1; v172++) {	// L370
    for (int v173 = 0; v173 < 1; v173++) {	// L371
      for (int v174 = 0; v174 < 1; v174++) {	// L372
        for (int v175 = 0; v175 < 32; v175++) {	// L373
          for (int v176 = 0; v176 < 1; v176++) {	// L374
            for (int v177 = 0; v177 < 2; v177++) {	// L375
            #pragma HLS pipeline II=4
              ap_int<512> v178 = 0;
              for (int v179 = 0; v179 < 4; v179++) {	// L377
              #pragma HLS pipeline II=1
                ap_int<128> v180 = v170.read(); //v170                v180 = v170;	// L378
                int v181 = ((v179 * 128) + 127);	// L379
                int v182 = (v179 * 128);	// L380
                v178(v181, v182) = v180;	// L381
              }
              v171.write(v178); //v171              v171 = v178;	// L383
            }
          }
        }
      }
    }
  }
}

void store0_0_top(
  hls::stream< ap_int<128> > &v183 /* v183[1] */,
  hls::stream< ap_int<512> > &v184 /* v184[1] */,
  hls::stream< ap_int<128> > &v185 /* v185[1] */,
  hls::stream< ap_int<512> > &v186 /* v186[1] */,
  hls::stream< ap_int<128> > &v187 /* v187[1] */,
  hls::stream< ap_int<512> > &v188 /* v188[1] */,
  hls::stream< ap_int<128> > &v189 /* v189[1] */,
  hls::stream< ap_int<512> > &v190 /* v190[1] */
){
  #pragma HLS inline OFF
  store0_0<0>(v183, v184);	// L393
  store0_0<1>(v185, v186);	// L394
  store0_0<2>(v187, v188);	// L395
  store0_0<3>(v189, v190);	// L396
}

template<int NC>
void store0(
  ap_int<512> v191[64][4],
  hls::stream< ap_int<512> > &v192 /* v192[1] */,
  hls::stream< ap_int<512> > &v193 /* v193[1] */,
  hls::stream< ap_int<512> > &v194 /* v194[1] */,
  hls::stream< ap_int<512> > &v195 /* v195[1] */
){
  #pragma HLS inline OFF
  for (int v196 = 0; v196 < 1; v196++) {	// L401
    for (int v197 = 0; v197 < 1; v197++) {	// L402
      for (int v198 = 0; v198 < 1; v198++) {	// L403
        for (int v199 = 0; v199 < 32; v199++) {	// L404
          for (int v200 = 0; v200 < 1; v200++) {	// L405
            for (int v201 = 0; v201 < 4; v201++) {	// L406
            #pragma HLS pipeline II=1
              bool v202 = v201 < 2;	// L407
              ap_int<512> v203;
              if (v202) {	// L408
                ap_int<512> v204 = v193.read(); //v193                v204 = v193;	// L409
                v203 = v204;	// L410
              } else {
                ap_int<512> v205 = v192.read(); //v192                v205 = v192;	// L412
                v203 = v205;	// L413
              }
              v191[((v199 + (v198 * 352)) + (v196 * 1408))][((v201 + (v200 * 16)) + (v197 * 96))] = v203;	// L415
            }
          }
        }
      }
      for (int v206 = 0; v206 < 1; v206++) {	// L420
        for (int v207 = 0; v207 < 32; v207++) {	// L421
          for (int v208 = 0; v208 < 1; v208++) {	// L422
            for (int v209 = 0; v209 < 4; v209++) {	// L423
            #pragma HLS pipeline II=1
              bool v210 = v209 < 2;	// L424
              ap_int<512> v211;
              if (v210) {	// L425
                ap_int<512> v212 = v195.read(); //v195                v212 = v195;	// L426
                v211 = v212;	// L427
              } else {
                ap_int<512> v213 = v194.read(); //v194                v213 = v194;	// L429
                v211 = v213;	// L430
              }
              v191[(((v207 + (v206 * 352)) + (v196 * 1408)) + 32)][((v209 + (v208 * 16)) + (v197 * 96))] = v211;	// L432
            }
          }
        }
      }
    }
  }
}

void store0_top(
  ap_int<512> v214[64][4],
  hls::stream< ap_int<512> > &v215 /* v215[1] */,
  hls::stream< ap_int<512> > &v216 /* v216[1] */,
  hls::stream< ap_int<512> > &v217 /* v217[1] */,
  hls::stream< ap_int<512> > &v218 /* v218[1] */
){
  #pragma HLS inline OFF
  store0<0>(v214, v215, v216, v217, v218);	// L442
}

template<int NC>
void load0(
  ap_int<512> v219[64][4],
  hls::stream< ap_int<512> > &v220 /* v220[1] */,
  hls::stream< ap_int<512> > &v221 /* v221[1] */,
  hls::stream< ap_int<512> > &v222 /* v222[1] */,
  hls::stream< ap_int<512> > &v223 /* v223[1] */
){
  #pragma HLS inline OFF
  for (int v224 = 0; v224 < 1; v224++) {	// L447
    for (int v225 = 0; v225 < 1; v225++) {	// L448
      for (int v226 = 0; v226 < 1; v226++) {	// L449
        for (int v227 = 0; v227 < 1; v227++) {	// L450
          for (int v228 = 0; v228 < 32; v228++) {	// L451
            for (int v229 = 0; v229 < 1; v229++) {	// L452
              for (int v230 = 0; v230 < 4; v230++) {	// L453
              #pragma HLS pipeline II=1
                ap_int<512> v231 = v219[((v228 + (v227 * 352)) + (v224 * 1408))][((v230 + (v229 * 8)) + (v226 * 8))];	// L454
                bool v232 = v230 < 2;	// L455
                if (v232) {	// L456
                  v220.write(v231); //v220                  v220 = v231;	// L457
                } else {
                  v221.write(v231); //v221                  v221 = v231;	// L459
                }
              }
            }
          }
        }
        for (int v233 = 0; v233 < 1; v233++) {	// L465
          for (int v234 = 0; v234 < 32; v234++) {	// L466
            for (int v235 = 0; v235 < 1; v235++) {	// L467
              for (int v236 = 0; v236 < 4; v236++) {	// L468
              #pragma HLS pipeline II=1
                ap_int<512> v237 = v219[(((v234 + (v233 * 352)) + (v224 * 1408)) + 32)][((v236 + (v235 * 8)) + (v226 * 8))];	// L469
                bool v238 = v236 < 2;	// L470
                if (v238) {	// L471
                  v222.write(v237); //v222                  v222 = v237;	// L472
                } else {
                  v223.write(v237); //v223                  v223 = v237;	// L474
                }
              }
            }
          }
        }
      }
    }
  }
}

void load0_top(
  ap_int<512> v239[64][4],
  hls::stream< ap_int<512> > &v240 /* v240[1] */,
  hls::stream< ap_int<512> > &v241 /* v241[1] */,
  hls::stream< ap_int<512> > &v242 /* v242[1] */,
  hls::stream< ap_int<512> > &v243 /* v243[1] */
){
  #pragma HLS inline OFF
  load0<0>(v239, v240, v241, v242, v243);	// L486
}

template<int NC>
void load0_3(
  hls::stream< ap_int<512> > &v244 /* v244[1] */,
  hls::stream< ap_int<128> > &v245 /* v245[1] */
){
  #pragma HLS inline OFF
  for (int v246 = 0; v246 < 1; v246++) {	// L490
    for (int v247 = 0; v247 < 1; v247++) {	// L491
      for (int v248 = 0; v248 < 1; v248++) {	// L492
        for (int v249 = 0; v249 < 1; v249++) {	// L493
          for (int v250 = 0; v250 < 32; v250++) {	// L494
            for (int v251 = 0; v251 < 1; v251++) {	// L495
              for (int v252 = 0; v252 < 2; v252++) {	// L496
              #pragma HLS pipeline II=4
                ap_int<512> v253 = v244.read(); //v244                v253 = v244;	// L497
                for (int v254 = 0; v254 < 4; v254++) {	// L498
                #pragma HLS pipeline II=1
                  int v255 = ((v254 * 128) + 127);	// L499
                  int v256 = (v254 * 128);	// L500
                  ap_int<128> v257 = v253(v255, v256);	// L501
                  v245.write(v257); //v245                  v245 = v257;	// L502
                }
              }
            }
          }
        }
      }
    }
  }
}

void load0_3_top(
  hls::stream< ap_int<512> > &v258 /* v258[1] */,
  hls::stream< ap_int<128> > &v259 /* v259[1] */,
  hls::stream< ap_int<512> > &v260 /* v260[1] */,
  hls::stream< ap_int<128> > &v261 /* v261[1] */,
  hls::stream< ap_int<512> > &v262 /* v262[1] */,
  hls::stream< ap_int<128> > &v263 /* v263[1] */,
  hls::stream< ap_int<512> > &v264 /* v264[1] */,
  hls::stream< ap_int<128> > &v265 /* v265[1] */,
  hls::stream< ap_int<512> > &v266 /* v266[1] */,
  hls::stream< ap_int<128> > &v267 /* v267[1] */,
  hls::stream< ap_int<512> > &v268 /* v268[1] */,
  hls::stream< ap_int<128> > &v269 /* v269[1] */,
  hls::stream< ap_int<512> > &v270 /* v270[1] */,
  hls::stream< ap_int<128> > &v271 /* v271[1] */,
  hls::stream< ap_int<512> > &v272 /* v272[1] */,
  hls::stream< ap_int<128> > &v273 /* v273[1] */
){
  #pragma HLS inline OFF
  load0_3<0>(v258, v259);	// L514
  load0_3<1>(v260, v261);	// L515
  load0_3<2>(v262, v263);	// L516
  load0_3<3>(v264, v265);	// L517
  load0_3<4>(v266, v267);	// L518
  load0_3<5>(v268, v269);	// L519
  load0_3<6>(v270, v271);	// L520
  load0_3<7>(v272, v273);	// L521
}

template<int NC>
void load1(
  ap_int<512> v274[64][4],
  hls::stream< ap_int<512> > &v275 /* v275[1] */,
  hls::stream< ap_int<512> > &v276 /* v276[1] */,
  hls::stream< ap_int<512> > &v277 /* v277[1] */,
  hls::stream< ap_int<512> > &v278 /* v278[1] */
){
  #pragma HLS inline OFF
  for (int v279 = 0; v279 < 1; v279++) {	// L526
    for (int v280 = 0; v280 < 1; v280++) {	// L527
      for (int v281 = 0; v281 < 1; v281++) {	// L528
        for (int v282 = 0; v282 < 1; v282++) {	// L529
          for (int v283 = 0; v283 < 32; v283++) {	// L530
            for (int v284 = 0; v284 < 1; v284++) {	// L531
              for (int v285 = 0; v285 < 4; v285++) {	// L532
              #pragma HLS pipeline II=1
                ap_int<512> v286 = v274[((v283 + (v282 * 128)) + (v281 * 128))][((v285 + (v284 * 16)) + (v280 * 96))];	// L533
                bool v287 = v285 < 2;	// L534
                if (v287) {	// L535
                  v277.write(v286); //v277                  v277 = v286;	// L536
                } else {
                  v276.write(v286); //v276                  v276 = v286;	// L538
                }
              }
            }
          }
        }
        for (int v288 = 0; v288 < 1; v288++) {	// L544
          for (int v289 = 0; v289 < 32; v289++) {	// L545
            for (int v290 = 0; v290 < 1; v290++) {	// L546
              for (int v291 = 0; v291 < 4; v291++) {	// L547
              #pragma HLS pipeline II=1
                ap_int<512> v292 = v274[(((v289 + (v288 * 128)) + (v281 * 128)) + 32)][((v291 + (v290 * 16)) + (v280 * 96))];	// L548
                bool v293 = v291 < 2;	// L549
                if (v293) {	// L550
                  v278.write(v292); //v278                  v278 = v292;	// L551
                } else {
                  v275.write(v292); //v275                  v275 = v292;	// L553
                }
              }
            }
          }
        }
      }
    }
  }
}

void load1_top(
  ap_int<512> v294[64][4],
  hls::stream< ap_int<512> > &v295 /* v295[1] */,
  hls::stream< ap_int<512> > &v296 /* v296[1] */,
  hls::stream< ap_int<512> > &v297 /* v297[1] */,
  hls::stream< ap_int<512> > &v298 /* v298[1] */
){
  #pragma HLS inline OFF
  load1<0>(v294, v295, v296, v297, v298);	// L565
}

void gemm_pl(
  ap_int<512> v299[64][4],
  ap_int<512> v300[64][4],
  ap_int<512> v301[64][4],
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v302 /* v302[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v303 /* v303[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v304 /* v304[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v305 /* v305[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v306 /* v306[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v307 /* v307[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v308 /* v308[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v309 /* v309[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v310 /* v310[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v311 /* v311[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v312 /* v312[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v313 /* v313[1] */
){
  #pragma HLS dataflow
  #pragma HLS inline OFF
  hls::stream< ap_int<128> > v314 /* v314[1] */;	// L569
  hls::stream< ap_int<128> > v315 /* v315[1] */;	// L570
  hls::stream< ap_int<128> > v316 /* v316[1] */;	// L571
  hls::stream< ap_int<128> > v317 /* v317[1] */;	// L572
  hls::stream< ap_int<128> > v318 /* v318[1] */;	// L573
  hls::stream< ap_int<128> > v319 /* v319[1] */;	// L574
  hls::stream< ap_int<128> > v320 /* v320[1] */;	// L575
  hls::stream< ap_int<128> > v321 /* v321[1] */;	// L576
  hls::stream< ap_int<128> > v322 /* v322[1] */;	// L577
  hls::stream< ap_int<128> > v323 /* v323[1] */;	// L578
  hls::stream< ap_int<128> > v324 /* v324[1] */;	// L579
  hls::stream< ap_int<128> > v325 /* v325[1] */;	// L580
  hls::stream< ap_int<512> > v326 /* v326[1] */;	// L581
  #pragma HLS stream variable=v326 depth=2
  hls::stream< ap_int<512> > v327 /* v327[1] */;	// L582
  #pragma HLS stream variable=v327 depth=2
  hls::stream< ap_int<512> > v328 /* v328[1] */;	// L583
  #pragma HLS stream variable=v328 depth=2
  hls::stream< ap_int<512> > v329 /* v329[1] */;	// L584
  #pragma HLS stream variable=v329 depth=2
  hls::stream< ap_int<512> > v330 /* v330[1] */;	// L585
  #pragma HLS stream variable=v330 depth=2
  hls::stream< ap_int<512> > v331 /* v331[1] */;	// L586
  #pragma HLS stream variable=v331 depth=2
  hls::stream< ap_int<512> > v332 /* v332[1] */;	// L587
  #pragma HLS stream variable=v332 depth=2
  hls::stream< ap_int<512> > v333 /* v333[1] */;	// L588
  #pragma HLS stream variable=v333 depth=2
  hls::stream< ap_int<512> > v334 /* v334[1] */;	// L589
  #pragma HLS stream variable=v334 depth=2
  hls::stream< ap_int<512> > v335 /* v335[1] */;	// L590
  #pragma HLS stream variable=v335 depth=2
  hls::stream< ap_int<512> > v336 /* v336[1] */;	// L591
  #pragma HLS stream variable=v336 depth=2
  hls::stream< ap_int<512> > v337 /* v337[1] */;	// L592
  #pragma HLS stream variable=v337 depth=2
  send3_top(v302, v322, v311, v320, v306, v324, v313, v321);	// L593
  receive2_top(v310, v316, v307, v314, v309, v317, v308, v315);	// L594
  send6_top(v304, v319, v305, v325, v303, v318, v312, v323);	// L595
  store0_0_top(v314, v326, v315, v327, v316, v328, v317, v329);	// L596
  store0_top(v301, v327, v326, v329, v328);	// L597
  load0_top(v299, v333, v332, v331, v330);	// L598
  load0_3_top(v333, v325, v332, v323, v331, v319, v330, v318, v337, v324, v336, v322, v335, v321, v334, v320);	// L599
  load1_top(v300, v334, v335, v337, v336);	// L600
}

void top(
  ap_int<512> v338[64][4],
  ap_int<512> v339[64][4],
  ap_int<512> v340[64][4],
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v341 /* v341[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v342 /* v342[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v343 /* v343[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v344 /* v344[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v345 /* v345[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v346 /* v346[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v347 /* v347[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v348 /* v348[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v349 /* v349[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v350 /* v350[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v351 /* v351[1] */,
  hls::stream< ap_axiu<128, 0 ,0 ,0> > &v352 /* v352[1] */
){
  #pragma HLS interface s_axilite port=return bundle=control
  #pragma HLS interface m_axi offset=slave bundle=gmem0 port=v338
  #pragma HLS interface s_axilite bundle=control port=v338
  #pragma HLS interface m_axi offset=slave bundle=gmem1 port=v339
  #pragma HLS interface s_axilite bundle=control port=v339
  #pragma HLS interface m_axi offset=slave bundle=gmem2 port=v340
  #pragma HLS interface s_axilite bundle=control port=v340
  #pragma HLS interface axis port=v341
  #pragma HLS interface axis port=v342
  #pragma HLS interface axis port=v343
  #pragma HLS interface axis port=v344
  #pragma HLS interface axis port=v345
  #pragma HLS interface axis port=v346
  #pragma HLS interface axis port=v347
  #pragma HLS interface axis port=v348
  #pragma HLS interface axis port=v349
  #pragma HLS interface axis port=v350
  #pragma HLS interface axis port=v351
  #pragma HLS interface axis port=v352

  gemm_pl(v338, v339, v340, v341, v342, v343, v344, v345, v346, v347, v348, v349, v350, v351, v352);	// L639
}


