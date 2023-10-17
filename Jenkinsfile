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
    stage('Build and Test') {
      when {
        expression { !env.GH_LABEL_DOC_ONLY.toBoolean() }
      }
      agent {
        label 'x86_64&&docker' // These agents have 24 cores so good for parallel xsim runs
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
        stage('Library checks') {
          steps {
            dir("${REPO}") {
              sh 'git clone git@github.com:xmos/infr_apps.git'
              sh 'git clone git@github.com:xmos/infr_scripts_py.git'
              // These are needed for xmake legacy build and also changelog check
              sh 'git clone git@github.com:xmos/lib_logging.git'
              sh 'git clone git@github.com:xmos/lib_xassert.git'
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
        stage('Run doc python') {
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
        stage('Tests XS2') {
          steps {
            runningOn(env.NODE_NAME)
            dir("${REPO}") {
              withTools(params.TOOLS_VERSION) {
                withVenv {
                  sh 'mkdir -p build'
                  dir("build") {
                    sh 'rm CMakeCache.txt'
                    sh 'cmake --toolchain ../xmos_cmake_toolchain/xs2a.cmake ..'
                    sh 'make test_ds3_voice test_us3_voice test_unity_gain_voice -j'
                  }
                  dir("tests") {
                    localRunPytest('-n auto -k "xs2" -vv')
                  }
                  dir("build") {
                    sh 'rm CMakeCache.txt' // Cleanup XS2 cmake cache for next stage
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
                    sh 'tree'
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
