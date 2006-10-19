Imports Tva.Security.Application
Imports Tva.Data.Common
Imports System.Data
Imports System.Data.SqlClient

Partial Class ChangePassword
    Inherits System.Web.UI.Page

    Private conn As New SqlConnection

    Protected Sub ButtonSubmit_Click(ByVal sender As Object, ByVal e As System.EventArgs) Handles ButtonSubmit.Click
        Dim userName As String = Me.TextBoxUserName.Text.Replace("'", "").Replace("%", "")
        Dim oldPassword As String = Me.TextBoxPassword.Text.Replace("'", "").Replace("%", "")
        Dim newPassword As String = Me.TextBoxNewPassword.Text.Replace("'", "").Replace("%", "")

        If Page.IsValid Then
            Try
                If Session("ConnectionString") Is Nothing Then
                    Response.Redirect("ErrorPage.aspx?t=1", True)
                End If

                If Session("ApplicationName") Is Nothing Then
                    Response.Redirect("ErrorPage.aspx?t=1", True)
                End If

                conn = New SqlConnection(Session("ConnectionString"))

                With New User(userName, oldPassword, New SqlConnection(Session("ConnectionString")))
                    If .IsExternal AndAlso .IsAuthenticated Then


                        If Not conn.State = ConnectionState.Open Then
                            conn.Open()
                        End If

                        ExecuteNonQuery("ChangePassword", conn, userName, Tva.Security.Application.User.EncryptPassword(oldPassword), Tva.Security.Application.User.EncryptPassword(newPassword))

                        Me.LabelError.Text = "Your password has been changed successfully. Please <a href=Login.aspx>login</a> with your new password."

                    Else
                        Me.LabelError.Text = "This is a TVA internal user name. Internal users cannot change their password here."
                    End If
                End With

            Catch sqlEx As SqlException
                If sqlEx.Number = 18456 Then
                    Me.LabelError.Text = "Error occured while connecting to the database. We will not be able to process your request. Please try again later."
                Else
                    'Log into the error log.
                    If Not conn.State = ConnectionState.Open Then
                        conn.Open()
                    End If
                    
                    ExecuteNonQuery("LogError", conn, Session("ApplicationName"), "ChangePassword.aspx Submit()", sqlEx.ToString)

                    Response.Redirect("ErrorPage.aspx", False)
                End If

            Catch ex As Exception
                'Log into the error log.
                If Not conn.State = ConnectionState.Open Then
                    conn.Open()
                End If

                ExecuteNonQuery("LogError", conn, Session("ApplicationName"), "ChangePassword.aspx Submit()", ex.ToString)

                Response.Redirect("ErrorPage.aspx", False)

            Finally
                If conn.State = ConnectionState.Open Then
                    conn.Close()
                End If

            End Try

        Else
            Response.Redirect("ErrorPage.aspx", False)

        End If

    End Sub

    Protected Sub Page_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load
        Me.TextBoxUserName.Focus()
        If Request("m") IsNot Nothing AndAlso _
                Not String.IsNullOrEmpty(Request("m").ToString()) Then

            Select Case CType(Request("m").ToString(), MessageTypes)

                Case MessageTypes.PasswordExpiredOrReset
                    Me.LabelError.Text = "Your password has expired or reset. Please change your password."

                Case Else
                    Me.LabelError.Text = ""

            End Select

        End If
    End Sub

    Private Enum MessageTypes As Integer
        PasswordExpiredOrReset

    End Enum
End Class
