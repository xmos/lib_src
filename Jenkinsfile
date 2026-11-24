// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.56.0') _

getApproval()

def withResetXTAG(String target, Closure body) {
    // Clear stale acquisition state left by a failed run before resetting.
    sh 'rm -f ~/.xtag/acquired'
    sh "xtagctl reset_all ${target}"
    withXTAG([target], body)
}

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
            description: 'The XCORE-400 (VX) tools version (bob)'
        )
        string(
            name: 'TOOLS_VERSION_VX_XCC',
            defaultValue: 'slipgate@master:143',
            description: 'The XCORE-400 (VX) XTC tools version (slipgate)'
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

                        stage('Legacy xcommon build') {
                            steps {
                                dir("${REPO_NAME}/tests/sim_tests/legacy_xcommon_test") {
                                    withTools(params.TOOLS_VERSION_XS) {
                                        sh 'xmake'
                                        // xsim returns the application's exit code, so a test failure fails this step.
                                        sh 'xsim bin/legacy_xcommon_test.xe'
                                    }
                                }
                            }
                        }
                      
                        stage('Simulator tests') {
                            steps {
                                dir("${REPO_NAME}/tests") {
                                    withTools(params.TOOLS_VERSION_XS) {
                                        createVenv(reqFile: "requirements.txt")
                                        withVenv {
                                            dir("sim_tests") {
                                                sh "pytest -q ../test_resource_usage.py"
                                                sh "pytest -n 1 -m prepare --junitxml=pytest_result_prepare.xml" // Build stage
                                                sh "pytest -v -n auto -m main --junitxml=pytest_result_run.xml" // Run in parallel
                                                archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true
                                                stash name: 'mips_reports_xs3a', includes: 'mips_report_asrc_xs3a.csv,mips_report_ssrc_xs3a.csv'
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
                                        xcoreBuild()
                                        withVenv {
                                            withResetXTAG("XCORE-AI-EXPLORER") { xtagIds ->
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
                                        withResetXTAG("XCORE-AI-EXPLORER") { xtagIds ->
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

                stage('vx4 tests') {
                    agent {
                        label 'x86_64 && linux'
                    }
                    steps {
                        dir(REPO_NAME) {
                            checkoutScmShallow()
                        }
                        withTools(params.TOOLS_VERSION_VX) {
                            dir("${REPO_NAME}/tests/sim_tests/riscv32-xmos_test") {
                                xcoreBuild(archiveBins: false)
                                sh 'xsim bin/riscv32-xmos_test.xe'
                            }
                        }
                    }
                    post {
                        cleanup {
                            xcoreCleanSandbox()
                        }
                    }
                } // stage('vx4 tests')

                stage('Slipgate tests') {
                    agent {
                        label 'x86_64 && linux'
                    }
                    stages {
                        stage('Build examples') {
                            steps {
                                dir(REPO_NAME) {
                                    checkoutScmShallow()
                                }
                                dir("${REPO_NAME}/examples") {
                                    withTools(params.TOOLS_VERSION_VX_XCC) {
                                        xcoreBuild(archiveBins: false, cmakeOpts: '-DAPP_HW_TARGET=XK-EVK-XU416')
                                    }
                                }
                            }
                        }

                        stage('Build and test MRHF and asynchronous FIFO') {
                            steps {
                                withTools(params.TOOLS_VERSION_VX_XCC) {
                                    dir("${REPO_NAME}/tests/sim_tests/asrc_vpu_test") {
                                        xcoreBuild(archiveBins: false, cmakeOpts: '-DAPP_HW_TARGET=XK-EVK-XU416')
                                    }
                                    dir("${REPO_NAME}/tests/sim_tests/asrc_test") {
                                        xcoreBuild(archiveBins: false, cmakeOpts: '-DAPP_HW_TARGET=XK-EVK-XU416')
                                    }
                                    dir("${REPO_NAME}/tests/sim_tests/ssrc_test") {
                                        xcoreBuild(archiveBins: false, cmakeOpts: '-DAPP_HW_TARGET=XK-EVK-XU416')
                                    }
                                    dir("${REPO_NAME}/tests") {
                                        createVenv(reqFile: "requirements.txt")
                                        withVenv {
                                            withEnv(["ASRC_TEST_PROFILE=vx4b", "SSRC_TEST_PROFILE=vx4b"]) {
                                                dir("sim_tests") {
                                                    sh "pytest -v test_mrhf.py::test_asrc_profile_selftest --junitxml=pytest_vx4_profile.xml"
                                                    sh "pytest -v test_asynchronous_fifo.py::test_asynchronous_fifo_output --junitxml=pytest_vx4_fifo.xml"
                                                    sh "pytest -v 'test_mrhf.py::test_prepare[asrc]' --junitxml=pytest_vx4_asrc_prepare.xml"
                                                    sh "pytest -v -n auto test_mrhf.py::test_asrc --junitxml=pytest_vx4_asrc.xml"
                                                    sh "pytest -v 'test_mrhf.py::test_prepare[ssrc]' --junitxml=pytest_vx4_ssrc_prepare.xml"
                                                    sh "pytest -v -n auto test_mrhf.py::test_ssrc --junitxml=pytest_vx4_ssrc.xml"
                                                    archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true
                                                    stash name: 'mips_reports_vx4b', includes: 'mips_report_asrc_vx4b.csv,mips_report_ssrc_vx4b.csv'
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } // stages
                    post {
                        always {
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_profile.xml", allowEmptyResults: true
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_fifo.xml", allowEmptyResults: true
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_asrc_prepare.xml", allowEmptyResults: true
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_asrc.xml", allowEmptyResults: true
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_ssrc_prepare.xml", allowEmptyResults: true
                            junit testResults: "${REPO_NAME}/tests/sim_tests/pytest_vx4_ssrc.xml", allowEmptyResults: true
                        }
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

                    dir("tests/sim_tests") {
                        unstash 'mips_reports_xs3a'
                        unstash 'mips_reports_vx4b'
                    }
                    dir("doc/python") {
                        withTools(params.TOOLS_VERSION_XS) {
                            createVenv(reqFile: "requirements.txt")
                        }
                    }

                    warnError("Docs") {
                        buildDocs(xmosdocVenvPath: "doc/python")
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
