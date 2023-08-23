@Library('xmos_jenkins_shared_library@v0.25.0') _

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
  agent {
    label 'x86_64&&macOS' // These agents have 24 cores so good for parallel xsim runs
  }
  environment {
    REPO = 'lib_src'
    VIEW = getViewName(REPO)
    PYTHON_VERSION = "3.10.5"
    VENV_DIRNAME = ".venv"
  }
  options {
    skipDefaultCheckout()
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.2.1',
      description: 'The XTC tools version'
    )
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
    // stage('Library checks') {
    //   steps {
    //     xcoreLibraryChecks("${REPO}")
    //   }
    // }
    // stage('xCORE builds') {
    //   steps {
    //     dir("${REPO}") {
    //       xcoreAllAppsBuild('examples')
    //       xcoreAllAppNotesBuild('examples')
    //       dir("${REPO}") {
    //         runXdoc('doc')
    //       }
    //     }
    //     // Archive all the generated .pdf docs
    //     archiveArtifacts artifacts: "${REPO}/**/pdf/*.pdf", fingerprint: true, allowEmptyArchive: true
    //   }
    // }
    stage ("Create Python environment")
    {
      steps {
        dir("${REPO}") {
          createVenv('requirements.txt')
          withVenv {
            sh 'pip install -r requirements.txt'
          }
        }
      }
    }
    stage('Tests') {
      steps {
        runningOn(env.NODE_NAME)
        dir("${REPO}") {
          withTools(params.TOOLS_VERSION) {
            withVenv {
              dir("tests") {
                localRunPytest('-m prepare') // Do all pre work like building and generating golden ref where needed

                // ASRC and SSRC tests
                localRunPytest('-m main -n auto -k "mrhf" -vv')
                archiveArtifacts artifacts: "mips_report*.csv", allowEmptyArchive: true

                // VPU enabled ff3 and rat tests
                localRunPytest('-m main -k "vpu" -vv') // xdist not working yet so no -n auto
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
