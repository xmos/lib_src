// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.34.0') _

getApproval()

pipeline {
  agent none
    environment {
        REPO = 'lib_src'
    }
    options {
        buildDiscarder(xmosDiscardBuildSettings())
        skipDefaultCheckout()
        timestamps()
    }
    parameters {
        string(
          name: 'TOOLS_VERSION',
          defaultValue: '15.3.0',
          description: 'The XTC tools version'
        )
        string(
          name: 'XMOSDOC_VERSION',
          defaultValue: 'v6.1.2',
          description: 'The xmosdoc version'
        )
        string(
            name: 'INFR_APPS_VERSION',
            defaultValue: 'v2.0.1',
            description: 'The infr_apps version'
        )
    }
    stages {
        stage ('Build and test') {
            parallel {
                stage('Build and sim test') {
                    agent {
                        label 'x86_64 && linux'
                    }
                    stages {
                        stage('Build examples') {
                            steps {
                                println "Stage running on ${env.NODE_NAME}"
                                dir("${REPO}") {
                                    checkout scm
                                    dir("examples") {
                                        withTools(params.TOOLS_VERSION) {
                                            sh 'cmake -G "Unix Makefiles" -B build'
                                            sh 'xmake -C build -j 16'
                                        }
                                    }
                                } // dir("${REPO}")
                            } // steps
                        }  // stage('Build examples')

                        stage('Library checks') {
                            steps {
                                runLibraryChecks("${WORKSPACE}/${REPO}", "${params.INFR_APPS_VERSION}")
                            }
                        }

                        stage('Simulator tests') {
                            steps {
                                dir("${REPO}/tests") {
                                    withTools(params.TOOLS_VERSION) {
                                        createVenv(reqFile: "requirements.txt")
                                        withVenv {
                                            dir("sim_tests") {
                                                sh "pytest -n 1 -m prepare --junitxml=pytest_result_prepare.xml" // Build stage
                                                sh "pytest -v -n auto -m main --junitxml=pytest_result_run.xml" // Run in parallel
                                                archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true
                                                archiveArtifacts artifacts: "gprof_results*/*.png", allowEmptyArchive: true
                                            }
                                        }
                                    }
                                }
                            } // steps
                        } // stage('Simulator tests')
                    } // stages
                    post {
                        always {
                            junit "${REPO}/tests/sim_tests/pytest_result_prepare.xml"
                            junit "${REPO}/tests/sim_tests/pytest_result_run.xml"
                        }
                        cleanup {
                            xcoreCleanSandbox()
                        }
                    }
                } // stage('Build and sim test')

                stage('Hardware tests') {
                    agent {
                        label 'xcore.ai && uhubctl'
                    }
                    steps {
                        println "Stage running on ${env.NODE_NAME}"
                            sh 'git clone https://github0.xmos.com/xmos-int/xtagctl.git'
                            sh 'git -C xtagctl checkout v2.0.0'
                            dir("${REPO}") {
                                checkout scm
                                dir("tests") {
                                    createVenv(reqFile: "requirements.txt")
                                    dir("hw_tests") {
                                        withTools(params.TOOLS_VERSION) {
                                            sh "cmake -G 'Unix Makefiles' -B build"
                                            sh "xmake -C build -j 8"
                                            withVenv {
                                                sh "pip install -e ${WORKSPACE}/xtagctl"
                                                withXTAG(["XCORE-AI-EXPLORER"]) { xtagIds ->
                                                    sh "pytest -n1 --junitxml=pytest_hw.xml"
                                                    sh "xrun --xscope --adapter-id ${xtagIds[0]} asynchronous_fifo_asrc_test/bin/asynchronous_fifo_asrc_test.xe"
                                                }
                                            } // withVenv
                                        } // withTools
                                    } // dir(hw_tests")
                                } // dir(tests)
                            } // dir ("${REPO}")
                        } //steps
                        post {
                            always {
                                junit "${REPO}/tests/hw_tests/pytest_hw.xml"
                            }
                            cleanup {
                                xcoreCleanSandbox()
                            }
                        } // post
                    }  // stage('Hardware tests')

                    stage('SNR plots') {
                        agent {
                            label 'xcore.ai && uhubctl'
                        }
                        steps {
                            println "Stage running on ${env.NODE_NAME}"
                            sh 'git clone https://github0.xmos.com/xmos-int/xtagctl.git'
                            sh 'git -C xtagctl checkout v2.0.0'
                            dir("${REPO}") {
                                checkout scm
                                dir("doc/python") {
                                    createVenv(reqFile: "requirements.txt")
                                    withVenv {
                                        sh "pip install -e ${WORKSPACE}/xtagctl"
                                        withTools(params.TOOLS_VERSION) {
                                            sh "pip install git+ssh://git@github.com/xmos/xscope_fileio@v1.2.0"
                                            withXTAG(["XCORE-AI-EXPLORER"]) { xtagIds ->
                                                sh "python -m doc_asrc.py --adapter-id " + xtagIds[0]
                                                stash name: 'doc_asrc_output', includes: '_build/**'
                                            }
                                        } // withTools
                                    } // withVenv
                                } // dir("doc/python")
                            } // dir("${REPO}")
                        } // steps
                        post {
                            cleanup {
                                xcoreCleanSandbox()
                            }
                        } // post
                    }  // stage('SNR plots')

                    stage('Legacy CMake build') {
                    agent {
                        label 'x86_64 && linux'
                    }
                        steps {
                            println "Stage running on ${env.NODE_NAME}"
                            dir("${REPO}") {
                                checkout scm
                                sh "git clone git@github.com:xmos/xmos_cmake_toolchain.git --branch v1.0.0"
                                 withTools(params.TOOLS_VERSION) {
                                    sh 'cmake -G "Unix Makefiles" -B build_legacy_cmake -DCMAKE_TOOLCHAIN_FILE=xmos_cmake_toolchain/xs3a.cmake'
                                    sh 'xmake -C build_legacy_cmake lib_src'
                                }
                            } // dir("${REPO}")
                        } // steps
                        post {
                            cleanup {
                                xcoreCleanSandbox()
                            }
                        } // post
                    }  // stage('Legacy CMake build')

            } // parallel
        } // stage ('Build and test')

        // This stage needs to wait for characterisation plots so run it last
        stage('Build documentation') {
            agent {
                label 'x86_64&&docker'
            }
            steps {
                println "Stage running on ${env.NODE_NAME}"
                dir("${REPO}") {
                    checkout scm

                    dir("doc/python") {
                        unstash 'doc_asrc_output'
                    }

                    warnError("Docs") {
                        buildDocs()
                    }
                }
            }
            post {
                cleanup {
                    xcoreCleanSandbox()
                }
            } // post
        } // stage('Build Documentation')
    } // stages
} // pipeline
