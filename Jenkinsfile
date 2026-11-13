// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.56.0') _

getApproval()

pipeline {
  agent none
    options {
        buildDiscarder(xmosDiscardBuildSettings(onlyArtifacts = false))
        skipDefaultCheckout()
        timestamps()
    }
    parameters {
        string(
                    name: 'TOOLS_VERSION_XS',
          defaultValue: '15.3.1',
                    description: 'The xcore.ai (XS) XTC tools version'
                )
                string(
                    name: 'TOOLS_VERSION_VX',
                    defaultValue: 'bob@develop',
                    description: 'The XCORE-400 (VX) tools version'
                )
                string(
                    name: 'TOOLS_VERSION_VX_XCC',
                    defaultValue: 'slipgate@master:143',
                    description: 'The XCORE-400 (VX) XTC tools version'
        )
        string(
          name: 'XMOSDOC_VERSION',
          defaultValue: 'v8.1.2',
          description: 'The xmosdoc version'
        )
        string(
            name: 'INFR_APPS_VERSION',
            defaultValue: 'v3.6.0',
            description: 'The infr_apps version'
        )
    }
    stages {
        stage('Setup') {
            agent {
                label 'linux'
            }
            steps {
                script {
                    def (server, user, repo) = extractFromScmUrl()
                    env.REPO_NAME = repo
                }
            }
        }

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
                                dir(REPO_NAME) {
                                    checkoutScmShallow()
                                    dir("examples") {
                                        xcoreBuild()
                                    }
                                } // dir("${REPO_NAME}")
                            } // steps
                        }  // stage('Build examples')

                        stage('Library checks') {
                            steps {
                                warnError("Repo checks failed"){
                                    runRepoChecks("${WORKSPACE}/${REPO_NAME}")
                                }
                            }
                        }

                        stage("Archive sandbox") {
                            steps {
                                archiveSandbox(REPO_NAME)
                            }  
                        }
                      
                        stage('Simulator tests') {
                            steps {
                                dir("${REPO_NAME}/tests") {
                                    withTools(params.TOOLS_VERSION_XS) {
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
                            junit "${REPO_NAME}/tests/sim_tests/pytest_result_prepare.xml"
                            junit "${REPO_NAME}/tests/sim_tests/pytest_result_run.xml"
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
                        dir(REPO_NAME) {
                            checkoutScmShallow()
                            dir("tests") {
                                withTools(params.TOOLS_VERSION_XS) {
                                    createVenv(reqFile: "requirements.txt")
                                    dir("hw_tests") {
                                        sh "cmake -G 'Unix Makefiles' -B build"
                                        sh "xmake -C build -j 8"
                                        withVenv {
                                            withXTAG(["XCORE-AI-EXPLORER"]) { xtagIds ->
                                                sh "pytest -n1 --junitxml=pytest_hw.xml"
                                                sh "xrun --xscope --adapter-id ${xtagIds[0]} asynchronous_fifo_asrc_test/bin/asynchronous_fifo_asrc_test.xe"
                                            }
                                        } // withVenv
                                    } //  dir("hw_tests")
                                } // withTools
                            } // dir("tests")
                        } // dir (REPO_NAME)
                    } //steps
                    post {
                        always {
                            junit "${REPO_NAME}/tests/hw_tests/pytest_hw.xml"
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
                        dir(REPO_NAME) {
                            checkoutScmShallow()
                            dir("doc/python") {
                                withTools(params.TOOLS_VERSION_XS) {
                                    createVenv(reqFile: "requirements.txt")
                                    withVenv {
                                        withXTAG(["XCORE-AI-EXPLORER"]) { xtagIds ->
                                            sh "python -m doc_asrc.py --adapter-id " + xtagIds[0]
                                            stash name: 'doc_asrc_output', includes: '_build/**'
                                        }
                                    } // withVenv
                                } // withTools
                            } // dir("doc/python")
                        } // dir(REPO_NAME)
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
                        dir(REPO_NAME) {
                            checkoutScmShallow()
                            // Clone "Boston" cmake
                            sh "git clone git@github.com:xmos/xmos_cmake_toolchain.git --branch v1.0.0"
                            withTools(params.TOOLS_VERSION_XS) {
                                sh 'cmake -G "Unix Makefiles" -B build_legacy_cmake -DCMAKE_TOOLCHAIN_FILE=xmos_cmake_toolchain/xs3a.cmake'
                                sh 'xmake -C build_legacy_cmake lib_src'
                            }
                        } // dir(REPO_NAME)
                    } // steps
                    post {
                        cleanup {
                            xcoreCleanSandbox()
                        }
                    } // post
                }  // stage('Legacy CMake build')

                stage('vx4 tests') {
                    agent {
                        label 'x86_64 && linux'
                    }
                    steps {
                        withTools(params.TOOLS_VERSION_VX) {
                            // TODO vx4 tests will be added here.
                        }
                    }
                    post {
                        cleanup {
                            xcoreCleanSandbox()
                        }
                    }
                } // stage('Bob tests')

                stage('Slipgate tests') {
                    agent {
                        label 'x86_64 && linux'
                    }
                    steps {
                        withTools(params.TOOLS_VERSION_VX_XCC) {
                            // VX4 XTC tests will be added here.
                        }
                    }
                    post {
                        cleanup {
                            xcoreCleanSandbox()
                        }
                    }
                } // stage('Slipgate tests')
            } // parallel
        } // stage ('Build and test')

        // This stage needs to wait for characterisation plots so run it last
        stage('Build documentation') {
            agent {
                label 'x86_64&&docker'
            }
            steps {
                println "Stage running on ${env.NODE_NAME}"
                dir(REPO_NAME) {
                    checkoutScmShallow()

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

        stage('🚀 Release') {
            when {
                expression { triggerRelease.isReleasable() }
            }
            steps {
                triggerRelease()
            }
        }
    } // stages
} // pipeline
