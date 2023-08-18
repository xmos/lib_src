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
    label 'x86_64&&macOS'
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
    stage('Get view') {
      steps {
        xcorePrepareSandbox("${VIEW}", "${REPO}")
      }
    }
    stage('Library checks') {
      steps {
        xcoreLibraryChecks("${REPO}")
      }
    }
    stage('xCORE builds') {
      steps {
        dir("${REPO}") {
          xcoreAllAppsBuild('examples')
          xcoreAllAppNotesBuild('examples')
          dir("${REPO}") {
            runXdoc('doc')
          }
        }
        // Archive all the generated .pdf docs
        archiveArtifacts artifacts: "${REPO}/**/pdf/*.pdf", fingerprint: true, allowEmptyArchive: true
      }
    }
    stage ("Create Python environment")
    {
      steps {
        installPipfile(false)
      }
    }
    stage('Tests') {
      steps {
        withTools(params.TOOLS_VERSION) {
          withVenv {
            dir("tests") {
              localRunPytest('')
            }
          }
        }
      }
    }
  }
  post {
    success {
      updateViewfiles()
    }
    cleanup {
      xcoreCleanSandbox()
    }
  }
}
