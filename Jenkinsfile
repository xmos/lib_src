// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@develop') _

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
      defaultValue: 'v6.0.0',
      description: 'The xmosdoc version')
  }
  stages {
    stage ('LIB SRC') {
    parallel {
    stage('Build and sim test') {
      agent {
        label 'x86_64 && linux'
      }
      stages {
        stage('Simulator tests') {
          steps {
            println "Stage running on ${env.NODE_NAME}"
            dir("${REPO}") {
              checkout scm
              withTools(params.TOOLS_VERSION) {
                createVenv(reqFile: "requirements.txt")
                withVenv {
                  dir("tests/sim_tests") {
                    sh "pytest -n 1 -m prepare --junitxml=pytest_result.xml" // Build stage
                    sh "pytest -n auto -m main --junitxml=pytest_result.xml" // Run in parallel
                    archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true
                    archiveArtifacts artifacts: "gprof_results*/*.png", allowEmptyArchive: true
                  }
                }
              }
            }
          }
        }
      }
      post {
        always {
          junit "${REPO}/tests/sim_tests/pytest_result.xml"
        }
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }  // stage('Build and sim test')
    stage('Hardware tests + Gen SNR plots') {
      agent {
        label 'xcore.ai && uhubctl'
      }
      stages {
        stage('HW tests') {
          steps {
            println "Stage running on ${env.NODE_NAME}"
            sh 'git clone https://github0.xmos.com/xmos-int/xtagctl.git'
            dir("${REPO}") {
              checkout scm
              createVenv(reqFile: "requirements.txt")
            }
            dir("${REPO}/tests/hw_tests") {
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
            } // dir("${REPO}/tests/hw_tests")
          } //steps
        } // stage('HW tests')
        stage('Generate SNR plots') {
          steps {
            dir("${REPO}/doc/python") {
              withVenv {
                withTools(params.TOOLS_VERSION) {
                  sh "pip install git+ssh://git@github.com/xmos/xscope_fileio@develop"
                  withXTAG(["XCORE-AI-EXPLORER"]) { xtagIds ->
                    sh "python -m doc_asrc.py --adapter-id " + xtagIds[0]
                    stash name: 'doc_asrc_output', includes: '_build/**'
                  }
                } // withTools
              } // withVenv
            } // dir("${REPO}/doc/python")
          } // steps
        } // stage('Generate SNR plots')
      } //stages
      post {
        always {
          junit "${REPO}/tests/hw_tests/pytest_hw.xml"
        }
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }  // Hardware tests
    } // parallel
    }
  }
}
