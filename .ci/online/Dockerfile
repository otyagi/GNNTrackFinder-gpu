ARG TAG=12-fair202211.4-rocm5.4
ARG REGISTRY=hub.cbm.gsi.de/computing/images/online

FROM ${REGISTRY}/dev:${TAG} as build
  ARG SOURCE_DIR=/opt/cbm/src
  WORKDIR ${SOURCE_DIR}
  # Kaniko doesn't support RUN --mount, so we have to copy the source code instead
  COPY . .
  RUN .ci/online/scripts/install.sh

FROM ${REGISTRY}/runtime:${TAG}
  ARG CBMROOT_PATH=/opt/cbm/cbmroot
  ARG FAIRSOFT_PATH=/opt/cbm/fairsoft
  COPY --from=build ${CBMROOT_PATH}/bin ${CBMROOT_PATH}/bin
  COPY --from=build ${CBMROOT_PATH}/lib/*.so ${CBMROOT_PATH}/lib/
  # FIXME HACK FOR DC OCT 23
  COPY --from=build ${FAIRSOFT_PATH}/lib/libGenVector.so ${FAIRSOFT_PATH}/lib/.
  COPY --from=build ${FAIRSOFT_PATH}/lib/libGenVector.so.6.26 ${FAIRSOFT_PATH}/lib/.
  COPY --from=build ${FAIRSOFT_PATH}/lib/libGenVector.so.6.26.10 ${FAIRSOFT_PATH}/lib/.
  COPY --from=build ${FAIRSOFT_PATH}/lib/libCore.so ${FAIRSOFT_PATH}/lib/.
  COPY --from=build ${FAIRSOFT_PATH}/lib/libCore.so.6.26 ${FAIRSOFT_PATH}/lib/.
  COPY --from=build ${FAIRSOFT_PATH}/lib/libCore.so.6.26.10 ${FAIRSOFT_PATH}/lib/.

  ENV LD_LIBRARY_PATH=${CBMROOT_PATH}/lib:${FAIRSOFT_PATH}/lib:/opt/rocm/lib:${LD_LIBRARY_PATH}
  ENV PATH=${CBMROOT_PATH}/bin:${PATH}
  CMD ["cbmreco"]
