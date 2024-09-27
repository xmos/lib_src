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
    stage('Build and test') {
      agent {
        label 'x86_64 && linux'
      }
      stages {
        stage('Simulator tests') {
          steps {
            dir("${REPO}") {
              checkout scm
              withTools(params.TOOLS_VERSION) {
                createVenv(reqFile: "requirements.txt")
                withVenv {
                  dir("tests/sim_tests") {
                    sh "pytest -n 1 -m prepare --junitxml=pytest_result.xml" // Build stage
                    sh "pytest -n auto -m main --junitxml=pytest_result.xml" // Run in parallel

                  }
                }
              }
            }
          }
        }
      }
      post {
        always {
          junit "${REPO}/tests/pytest_result.xml"
        }
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }  // Build and test
  }
}
