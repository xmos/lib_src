@Library('xmos_jenkins_shared_library@v0.25.0')

def runningOn(machine) {
    println "Stage running on:"
    println machine
}

// run pytest with common flags for project. any passed in though extra args will
// be appended
def localRunPytest(String extra_args="") {
    catchError{
        sh "python -m pytest --junitxml=pytest_result.xml -rA -v --durations=0 -o junit_logging=all ${extra_args}"
    }
    junit "pytest_result.xml"
}

getApproval()

pipeline {
  agent none
  environment {
    REPO = 'lib_src'
    VIEW = getViewName(REPO)
    PYTHON_VERSION = "3.10.5"
    VENV_DIRNAME = ".venv"
    XMOSDOC_VERSION = "v4.0"
  }
  options {
    skipDefaultCheckout()
    timestamps()
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.2.1',
      description: 'The XTC tools version'
    )
  }
  stages {
    stage ('LIB SRC') {
      parallel {
        stage ('Generate SNR plots') {
          agent {
            label 'xcore.ai && uhubctl'
          }
          stages {
            stage('Get repo') {
              steps {
                runningOn(env.NODE_NAME)
                sh "mkdir ${REPO}"
                // source checks require the directory
                // name to be the same as the repo name
                dir("${REPO}") {
                  // checkout repo
                  checkout scm
                  sh 'git submodule update --init --recursive --depth 1'
                }
              }
            }
            stage ("Create Python environment") {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  createVenv('requirements.txt')
                  withVenv {
                    sh 'pip install -r requirements.txt'
                  }
                }
              }
            }
            stage ('Gen plots') {
              steps {
                runningOn(env.NODE_NAME)
                sh 'git clone https://github0.xmos.com/xmos-int/xtagctl.git'
                dir("lib_src") {
                  checkout scm
                  sh 'git submodule update --init --recursive'
                }
                createVenv("lib_src/requirements.txt")

                dir("lib_src") {
                  withVenv {
                    withTools(params.TOOLS_VERSION) {
                      sh "pip install -r requirements.txt"
                      sh "pip install -e ${WORKSPACE}/xtagctl"
                      withXTAG(["XCORE-AI-EXPLORER"]) { adapterIDs ->
                        sh "xtagctl reset ${adapterIDs[0]}"
                        dir("doc/python") {
                          sh "pip install -r requirements_test.txt"
                          sh "python -m doc_asrc.py --adapter-id " + adapterIDs[0]
                          stash name: 'doc_asrc_output', includes: '_build/**'
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // SNR plots

        stage ('Hardware Tests') {
          agent {
            label 'xcore.ai && uhubctl'
          }
          stages {
            stage('Get repo') {
              steps {
                runningOn(env.NODE_NAME)
                // source checks require the directory
                // name to be the same as the repo name
                dir("${REPO}") {
                  // checkout repo
                  checkout scm
                  sh 'git submodule update --init --recursive --depth 1'
                }
              }
            }
            stage ("Create Python environment") {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  createVenv('requirements.txt')
                  withVenv {
                    sh 'pip install -r requirements.txt'
                  }
                }
              }
            }
            stage ('Build and run tests') {
              steps {
                runningOn(env.NODE_NAME)
                sh 'git clone https://github0.xmos.com/xmos-int/xtagctl.git'
                dir("lib_src") {
                  checkout scm
                  sh 'git submodule update --init --recursive'
                  sh 'git clone git@github.com:xmos/lib_logging.git'
                }
                createVenv("lib_src/requirements.txt")

                dir("lib_src") {
                  withVenv {
                    withTools(params.TOOLS_VERSION) {
                      sh "pip install -r requirements.txt"
                      sh "pip install -e ${WORKSPACE}/xtagctl"
                      withXTAG(["XCORE-AI-EXPLORER"]) { adapterIDs ->
                        sh "xtagctl reset ${adapterIDs[0]}"
                        // Do asynch FIFO test
                        dir("tests/asynchronous_fifo_asrc_test") {
                          sh "xmake -j"
                          sh "xrun --xscope --adapter-id " + adapterIDs[0] + " bin/asynchronous_fifo_asrc_test.xe"
                        }
                        // ASRC Task tests
                        dir("tests") {
                          localRunPytest('-k "asrc_task" -vv')
                          archiveArtifacts artifacts: "*.png", allowEmptyArchive: true
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        } // HW tests

        stage('Sim tests') {
          when {
            expression { !env.GH_LABEL_DOC_ONLY.toBoolean() }
          }
          agent {
            label 'x86_64&&docker' // These agents have 24 cores so good for parallel xsim runs
          }
          stages {
            stage('Get repo') {
              steps {
                runningOn(env.NODE_NAME)
                sh "mkdir ${REPO}"
                // source checks require the directory
                // name to be the same as the repo name
                dir("${REPO}") {
                  // checkout repo
                  checkout scm
                  sh 'git submodule update --init --recursive --depth 1'
                }
              }
            }
            stage ("Create Python environment") {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  createVenv('requirements.txt')
                  withVenv {
                    sh 'pip install -r requirements.txt'
                  }
                }
              }
            }
            stage('Library checks') {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  sh 'git clone git@github.com:xmos/infr_apps.git'
                  sh 'git clone git@github.com:xmos/infr_scripts_py.git'
                  // These are needed for xmake legacy build and also changelog check
                  sh 'git clone git@github.com:xmos/lib_logging.git'
                  withVenv {
                    sh 'pip install -e infr_scripts_py'
                    sh 'pip install -e infr_apps'
                    dir("tests") {
                      withEnv(["XMOS_ROOT=.."]) {
                        localRunPytest('-s test_lib_checks.py -vv')
                      }
                    }
                  }
                }
              }
            }
            stage('Test xmake build') {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  withTools(params.TOOLS_VERSION) {
                    withVenv {
                      dir("tests") {
                        localRunPytest('-k "legacy" -vv')
                      }
                    }
                  }
                }
              }
            }
            stage('Tests XS2') {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  withTools(params.TOOLS_VERSION) {
                    withVenv {
                      sh 'mkdir -p build'
                      dir("build") {
                        sh 'rm -f -- CMakeCache.txt'
                        sh 'cmake --toolchain ../xmos_cmake_toolchain/xs2a.cmake ..'
                        sh 'make test_ds3_voice test_us3_voice test_unity_gain_voice -j'
                      }
                      dir("tests") {
                        localRunPytest('-n auto -k "xs2" -vv')
                      }
                      dir("build") {
                        sh 'rm -f -- CMakeCache.txt' // Cleanup XS2 cmake cache for next stage
                      }
                    }
                  }
                }
              }
            }
            stage('Tests XS3') {
              steps {
                runningOn(env.NODE_NAME)
                dir("${REPO}") {
                  withTools(params.TOOLS_VERSION) {
                    withVenv {
                      dir("tests") {
                        localRunPytest('-m prepare') // Do all pre work like building and generating golden ref where needed

                        // FF3 HiFi tests for OS3 and DS3
                        localRunPytest('-m main -n auto -k "hifi_ff3" -vv')

                        // ASRC and SSRC tests across all in/out freqs and deviations (asrc only)
                        localRunPytest('-m main -n auto -k "mrhf" -vv')
                        archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true

                        // VPU enabled ff3 and rat tests
                        localRunPytest('-m main -k "vpu" -vv') // xdist not working yet so no -n auto

                        // Profile the ASRC
                        localRunPytest('-m main -k "profile_asrc" -vv')
                        archiveArtifacts artifacts: "gprof_results/*.png", allowEmptyArchive: true
                      }
                    }
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        }
      }
    }
    // This stage needs to wait for characterisation plots so put at end
    stage('Documentation') {
      agent {
        label 'x86_64&&docker'
      }
      stages {
        stage('Get repo') {
          steps {
            sh "mkdir ${REPO}"
            // source checks require the directory
            // name to be the same as the repo name
            dir("${REPO}") {
              // checkout repo
              checkout scm
              sh 'git submodule update --init --recursive --depth 1'
            }
          }
        }
        stage ("Create Python environment") {
          steps {
            dir("${REPO}") {
              createVenv('requirements.txt')
              withVenv {
                sh 'pip install -r requirements.txt'
              }
            }
          }
        }
        stage('Unstash doc_asrc.py output') {
          steps {
            runningOn(env.NODE_NAME)
            dir("${REPO}/doc/python") {
              unstash 'doc_asrc_output'
            }
          }
        }
        stage('Build docs') {
          steps {
            runningOn(env.NODE_NAME)
            dir("${REPO}") {
              withTools(params.TOOLS_VERSION) {
                withVenv {
                  sh "sh doc/build_docs_ci.sh $XMOSDOC_VERSION"
                  archiveArtifacts artifacts: "doc_build.zip", allowEmptyArchive: true
                }
              }
            }
          }
        }
      }
      post {
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }
  }
}
